/*
 * =====================================================================================
 *
 *       Filename: mmapboard.cpp
 *        Created: 10/08/2017 19:22:30
 *    Description:
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#include <array>
#include "client.hpp"
#include "pngtexdb.hpp"
#include "sdldevice.hpp"
#include "mmapboard.hpp"
#include "maprecord.hpp"
#include "processrun.hpp"
#include "dbcomrecord.hpp"

extern PNGTexDB *g_progUseDB;
extern SDLDevice *g_sdlDevice;

MMapBoard::MMapBoard(ProcessRun *runPtr, Widget *parent, bool autoDelete)
    : Widget
      {
          DIR_UPLEFT,
          0,
          0,
          0,
          0,

          parent,
          autoDelete,
      }
    , m_processRun(runPtr)
    , m_buttonAlpha
      {
          DIR_UPLEFT,
          0,
          0,

          {
              0X09000002,
              0X09000002,
              0X09000002,
          },

          nullptr,
          nullptr,
          [this]()
          {
              m_alphaOn = !m_alphaOn;
              if(m_alphaOn){
                  m_buttonAlpha.setTexID({0X09000003, 0X09000003, 0X09000003});
              }
              else{
                  m_buttonAlpha.setTexID({0X09000002, 0X09000002, 0X09000002});
              }
          },

          0,
          0,
          0,
          0,

          false,
          true,
          this,
      }
    , m_buttonExtend
      {
          DIR_UPLEFT,
          0,
          0,

          {
              0X09000004,
              0X09000004,
              0X09000004,
          },

          nullptr,
          nullptr,
          [this]()
          {
              if(getMmapTexture()){
                  flipExtended();
              }

              if(m_extended){
                  m_buttonExtend.setTexID({0X09000005, 0X09000005, 0X09000005});
              }
              else{
                  m_buttonExtend.setTexID({0X09000004, 0X09000004, 0X09000004});
              }
          },

          0,
          0,
          0,
          0,

          false,
          true,
          this,
      }
{
    show(false);
}

void MMapBoard::drawEx(int, int, int, int, int, int) const
{
    drawMmapTexture();
    drawFrame();

    m_buttonAlpha.draw();
    m_buttonExtend.draw();
}

bool MMapBoard::processEvent(const SDL_Event &event, bool valid)
{
    if(!show()){
        return false;
    }

    if(!valid){
        m_buttonAlpha .setOff();
        m_buttonExtend.setOff();
        return false;
    }

    bool took = false;
    took |= m_buttonAlpha .processEvent(event, valid && !took);
    took |= m_buttonExtend.processEvent(event, valid && !took);
    return took;
}

void MMapBoard::flipExtended()
{
    m_extended = !m_extended;
    setLoc();
    m_buttonAlpha .setOff();
    m_buttonExtend.setOff();
}

void MMapBoard::setLoc()
{
    const int size = getFrameSize();
    m_w = size;
    m_h = size;

    moveTo(g_sdlDevice->getRendererWidth() - w(), 0);

    const int buttonW = std::max<int>(m_buttonAlpha.w(), m_buttonExtend.w());
    const int buttonH = std::max<int>(m_buttonAlpha.h(), m_buttonExtend.h());

    m_buttonAlpha .moveTo(w() - 2 * buttonW + 1, h() - buttonH);
    m_buttonExtend.moveTo(w() -     buttonW + 0, h() - buttonH);
}

void MMapBoard::drawMmapTexture() const
{
    auto texPtr = getMmapTexture();
    if(!texPtr){
        return;
    }

    const auto [mapID, mapW, mapH] = m_processRun->getMap();
    const auto [texW, texH] = SDLDeviceHelper::getTextureSize(texPtr);
    const auto fnGetMPLoc = [mapW, mapH, texW, texH](const std::tuple<int, int> &loc) -> std::tuple<int, int>
    {
        return
        {
            (int)(std::lround((std::get<0>(loc) * 1.0 / mapW) * texW)),
            (int)(std::lround((std::get<1>(loc) * 1.0 / mapH) * texH)),
        };
    };

    if(!m_alphaOn){
        g_sdlDevice->fillRectangle(colorf::BLACK + 255, x(), y(), w(), h());
    }

    const auto [heroMPX, heroMPY] = fnGetMPLoc(m_processRun->getMyHero()->location());
    const int srcX = std::min<int>(std::max<int>(0, heroMPX - w() / 2), texW - w());
    const int srcY = std::min<int>(std::max<int>(0, heroMPY - h() / 2), texH - h());
    {
        SDLDeviceHelper::EnableTextureModColor enableModColor(texPtr, colorf::WHITE + (m_alphaOn ? 200 : 255));
        g_sdlDevice->drawTexture(texPtr, x(), y(), srcX, srcY, w(), h());
    }

    for(const auto &p: m_processRun->getCOList()){
        const auto [coMPX, coMPY] = fnGetMPLoc(p.second->location());
        const auto [color, r] = [this](uint64_t uid) -> std::tuple<uint32_t, int>
        {
            switch(uidf::getUIDType(uid)){
                case UID_PLY:
                    {
                        if(uid == m_processRun->getMyHeroUID()){
                            return {colorf::RGBA(255, 0, 255, 255), 2};
                        }
                        else{
                            return {colorf::RGBA(200, 0, 200, 255), 2};
                        }
                    }
                case UID_NPC:
                    {
                        return {colorf::BLUE+ 255, 2};
                    }
                case UID_MON:
                    {
                        return {colorf::RED + 255, 1};
                    }
                default:
                    {
                        return {0, 0};
                    }
            }
        }(p.first);

        if(colorf::A(color)){
            g_sdlDevice->fillRectangle(color, x() + (coMPX - srcX) - r, y() + (coMPY - srcY) - r, 2 * r + 1, 2 * r + 1);
        }
    }
}

void MMapBoard::drawFrame() const
{
    g_sdlDevice->drawRectangle(colorf::RGBA(60, 60, 60, 255), x(), y(), w(), h());
    if(auto texPtr = g_progUseDB->Retrieve(0X09000006); texPtr){
        g_sdlDevice->drawTexture(texPtr, x(), y());
    }

    if(auto texPtr = g_progUseDB->Retrieve(0X09000007); texPtr){
        g_sdlDevice->drawTexture(texPtr, x() + w() - SDLDeviceHelper::getTextureWidth(texPtr), y());
    }

    if(auto texPtr = g_progUseDB->Retrieve(0X09000008); texPtr){
        g_sdlDevice->drawTexture(texPtr, x(), y() + h() - SDLDeviceHelper::getTextureHeight(texPtr));
    }
}

int MMapBoard::getFrameSize() const
{
    const auto [texW, texH] = SDLDeviceHelper::getTextureSize(getMmapTexture());
    if(m_extended){
        return std::min<int>({texW, texH, 300}); // setup window size here
    }else{
        return std::min<int>({texW, texH, 128});
    }
}

SDL_Texture *MMapBoard::getMmapTexture() const
{
    [[maybe_unused]] const auto [mapID, mapW, mapH] = m_processRun->getMap();
    if(const auto miniMapID = DBCOM_MAPRECORD(mapID).miniMapID){
        return g_progUseDB->Retrieve(miniMapID);
    }
    return nullptr;
}

void MMapBoard::flipMmapShow()
{
    flipShow(this);
    setLoc();
}

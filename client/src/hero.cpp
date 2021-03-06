/*
 * =====================================================================================
 *
 *       Filename: hero.cpp
 *        Created: 09/03/2015 03:49:00
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

#include "log.hpp"
#include "hero.hpp"
#include "pathf.hpp"
#include "dbcomid.hpp"
#include "mathf.hpp"
#include "sysconst.hpp"
#include "pngtexdb.hpp"
#include "sdldevice.hpp"
#include "processrun.hpp"
#include "motionnode.hpp"
#include "attachmagic.hpp"
#include "dbcomrecord.hpp"
#include "pngtexoffdb.hpp"
#include "dbcomrecord.hpp"
#include "clientargparser.hpp"

extern Log *g_log;
extern SDLDevice *g_sdlDevice;
extern PNGTexDB *g_progUseDB;
extern PNGTexOffDB *g_heroDB;
extern PNGTexOffDB *g_hairDB;
extern PNGTexOffDB *g_weaponDB;
extern PNGTexOffDB *g_helmetDB;
extern ClientArgParser *g_clientArgParser;

Hero::Hero(uint64_t uid, ProcessRun *proc, const ActionNode &action)
    : CreatureMovable(uid, proc)
    , m_horse(0)
    , m_onHorse(false)
{
    m_currMotion.reset(new MotionNode
    {
        .type = MOTION_STAND,
        .direction = DIR_DOWN,
        .x = action.x,
        .y = action.y,
    });

    if(!parseAction(action)){
        throw fflerror("failed to parse action");
    }
}

void Hero::drawFrame(int viewX, int viewY, int, int frame, bool)
{
    const auto [shiftX, shiftY] = getShift(frame);
    const auto startX = m_currMotion->x * SYS_MAPGRIDXP + shiftX - viewX;
    const auto startY = m_currMotion->y * SYS_MAPGRIDYP + shiftY - viewY;

    const auto fnDrawWeapon = [startX, startY, frame, this](bool shadow)
    {
        // 04 - 00 :     frame : max =  32
        // 07 - 05 : direction : max =  08 : +
        // 13 - 08 :    motion : max =  64 : +
        // 21 - 14 :    weapon : max = 256 : +----> GfxWeaponID
        //      22 :    gender :
        //      23 :    shadow :
        const auto nGfxWeaponID = GfxWeaponID(DBCOM_ITEMRECORD(getWLItem(WLG_WEAPON).itemID).shape, m_currMotion->type, m_currMotion->direction);
        if(nGfxWeaponID < 0){
            return;
        }

        const uint32_t weaponKey = ((to_u32(shadow ? 1 : 0)) << 23) + (to_u32(gender()) << 22) + ((nGfxWeaponID & 0X01FFFF) << 5) + frame;
        const auto [weaponFrame, weaponDX, weaponDY] = g_weaponDB->Retrieve(weaponKey);

        if(weaponFrame && shadow){
            SDL_SetTextureAlphaMod(weaponFrame, 128);
        }
        g_sdlDevice->drawTexture(weaponFrame, startX + weaponDX, startY + weaponDY);
    };

    fnDrawWeapon(true);

    const auto nDress     = DBCOM_ITEMRECORD(getWLItem(WLG_DRESS).itemID).shape;
    const auto nMotion    = m_currMotion->type;
    const auto nDirection = m_currMotion->direction;

    const auto nGfxDressID = GfxDressID(nDress, nMotion, nDirection);
    if(nGfxDressID < 0){
        m_currMotion->print();
        return;
    }

    // human gfx dress id indexing
    // 04 - 00 :     frame : max =  32
    // 07 - 05 : direction : max =  08 : +
    // 13 - 08 :    motion : max =  64 : +
    // 21 - 14 :     dress : max = 256 : +----> GfxDressID
    //      22 :       sex :
    //      23 :    shadow :
    const uint32_t nKey0 = (to_u32(0) << 23) + (to_u32(gender()) << 22) + ((to_u32(nGfxDressID & 0X01FFFF)) << 5) + frame;
    const uint32_t nKey1 = (to_u32(1) << 23) + (to_u32(gender()) << 22) + ((to_u32(nGfxDressID & 0X01FFFF)) << 5) + frame;

    const auto [pFrame0, nDX0, nDY0] = g_heroDB->Retrieve(nKey0);
    const auto [pFrame1, nDX1, nDY1] = g_heroDB->Retrieve(nKey1);

    if(pFrame1){
        SDL_SetTextureAlphaMod(pFrame1, 128);
    }
    g_sdlDevice->drawTexture(pFrame1, startX + nDX1, startY + nDY1);

    if(true
            && getWLItem(WLG_WEAPON)
            && WeaponOrder(m_currMotion->type, m_currMotion->direction, frame) == 1){
        fnDrawWeapon(false);
    }

    for(auto &p: m_attachMagicList){
        switch(p->magicID()){
            case DBCOM_MAGICID(u8"魔法盾"):
                {
                    p->drawShift(startX, startY, true);
                    break;
                }
            default:
                {
                    p->drawShift(startX, startY, false);
                    break;
                }
        }
    }

    g_sdlDevice->drawTexture(pFrame0, startX + nDX0, startY + nDY0);
    if(getWLItem(WLG_HELMET)){
        if(const auto nHelmetGfxID = gfxHelmetID(DBCOM_ITEMRECORD(getWLItem(WLG_HELMET).itemID).shape, nMotion, nDirection); nHelmetGfxID >= 0){
            const uint32_t nHelmetKey = (to_u32(gender()) << 22) + ((to_u32(nHelmetGfxID & 0X01FFFF)) << 5) + frame;
            if(auto [texPtr, dx, dy] = g_helmetDB->Retrieve(nHelmetKey); texPtr){
                g_sdlDevice->drawTexture(texPtr, startX + dx, startY + dy);
            }
        }
    }
    else{
        if(const auto nHairGfxID = GfxHairID(m_sdWLDesp.hair, nMotion, nDirection); nHairGfxID >= 0){
            const uint32_t nHairKey = (to_u32(gender()) << 22) + ((to_u32(nHairGfxID & 0X01FFFF)) << 5) + frame;
            if(auto [texPtr, dx, dy] = g_hairDB->Retrieve(nHairKey); texPtr){
                SDLDeviceHelper::EnableTextureModColor enableColor(texPtr, m_sdWLDesp.hairColor);
                g_sdlDevice->drawTexture(texPtr, startX + dx, startY + dy);
            }
        }
    }

    if(true
            && getWLItem(WLG_WEAPON)
            && WeaponOrder(m_currMotion->type, m_currMotion->direction, frame) == 0){
        fnDrawWeapon(false);
    }

    if(g_clientArgParser->drawTextureAlignLine){
        g_sdlDevice->drawLine(colorf::RED + 128, startX, startY, startX + nDX0, startY + nDY0);
        g_sdlDevice->drawLine(colorf::BLUE + 128, startX - 5, startY, startX + 5, startY);
        g_sdlDevice->drawLine(colorf::BLUE + 128, startX, startY - 5, startX, startY + 5);
    }

    if(g_clientArgParser->drawTargetBox){
        if(const auto box = getTargetBox()){
            g_sdlDevice->drawRectangle(colorf::BLUE + 128, box.x - viewX, box.y - viewY, box.w, box.h);
        }
    }

    // draw attached magic for the second time
    // for some direction magic should be on top of body

    for(auto &p: m_attachMagicList){
        switch(p->magicID()){
            case DBCOM_MAGICID(u8"灵魂火符"):
                {
                    switch(m_currMotion->direction){
                        case DIR_UP:
                        case DIR_UPRIGHT:
                        case DIR_RIGHT:
                        case DIR_DOWNRIGHT:
                            {
                                break;
                            }
                        default:
                            {
                                p->drawShift(startX, startY, true);
                                break;
                            }
                    }
                    break;
                }
            default:
                {
                    p->drawShift(startX, startY, true);
                    break;
                }
        }
    }

    switch(m_currMotion->type){
        case MOTION_SPELL0:
        case MOTION_SPELL1:
            {
                if(m_currMotion->extParam.spell.effect){
                    m_currMotion->extParam.spell.effect->drawShift(startX, startY, false);
                }
                break;
            }
        case MOTION_ONEHSWING:
        case MOTION_ONEVSWING:
        case MOTION_TWOHSWING:
        case MOTION_TWOVSWING:
        case MOTION_RANDSWING:
        case MOTION_SPEARHSWING:
        case MOTION_SPEARVSWING:
            {
                if(m_currMotion->extParam.swing.magicID){
                    //
                }
                break;
            }
        default:
            {
                break;
            }
    }

    // draw HP bar
    // if current m_HPMqx is zero we draw full bar
    if(m_currMotion->type != MOTION_DIE && g_clientArgParser->drawHPBar){
        auto pBar0 = g_progUseDB->Retrieve(0X00000014);
        auto pBar1 = g_progUseDB->Retrieve(0X00000015);

        int nW = -1;
        int nH = -1;
        SDL_QueryTexture(pBar1, nullptr, nullptr, &nW, &nH);

        const int drawHPX = startX +  7;
        const int drawHPY = startY - 53;
        const int drawHPW = to_d(std::lround(nW * (m_maxHP ? (std::min<double>)(1.0, (1.0 * m_HP) / m_maxHP) : 1.0)));

        g_sdlDevice->drawTexture(pBar1, drawHPX, drawHPY, 0, 0, drawHPW, nH);
        g_sdlDevice->drawTexture(pBar0, drawHPX, drawHPY);
    }
}

bool Hero::update(double ms)
{
    updateAttachMagic(ms);
    m_currMotion->updateSpellEffect(ms);

    if(!checkUpdate(ms)){
        return true;
    }

    const CallOnExitHelper motionOnUpdate([this]()
    {
        m_currMotion->update();
    });

    switch(m_currMotion->type){
        case MOTION_STAND:
            {
                if(stayIdle()){
                    return advanceMotionFrame(1);
                }

                // move to next motion will reset frame as 0
                // if current there is no more motion pending
                // it will add a MOTION_STAND
                //
                // we don't want to reset the frame here
                return moveNextMotion();
            }
        case MOTION_HITTED:
            {
                if(stayIdle()){
                    return updateMotion();
                }

                // move to next motion will reset frame as 0
                // if current there is no more motion pending
                // it will add a MOTION_STAND
                //
                // we don't want to reset the frame here
                return moveNextMotion();
            }
        case MOTION_SPELL0:
        case MOTION_SPELL1:
            {
                if(!m_currMotion->extParam.spell.effect){
                    return updateMotion();
                }

                const int motionEndFrame = motionFrameCountEx(m_currMotion->type, m_currMotion->direction) - 1;
                const int effectEndFrame = m_currMotion->extParam.spell.effect->frameCount() - 1;
                const int motionSyncFrameCount = [this]() -> int
                {
                    if(m_currMotion->type == MOTION_SPELL0){
                        return 3;
                    }
                    return 1;
                }();
                const int effectSyncFrameCount = [motionSyncFrameCount, this]() -> int
                {
                    return std::lround(m_currMotion->extParam.spell.effect->speed() * motionSyncFrameCount / m_currMotion->speed);
                }();

                if( m_currMotion->frame >= motionEndFrame - motionSyncFrameCount && m_currMotion->extParam.spell.effect->frame() < effectEndFrame - effectSyncFrameCount){
                    m_currMotion->frame  = motionEndFrame - motionSyncFrameCount;
                    return true;
                }
                else{
                    return updateMotion();
                }
            }
        case MOTION_ONEHSWING:
        case MOTION_ONEVSWING:
        case MOTION_TWOHSWING:
        case MOTION_TWOVSWING:
        case MOTION_RANDSWING:
        case MOTION_SPEARHSWING:
        case MOTION_SPEARVSWING:
            {
                if(m_currMotion->extParam.swing.magicID){
                    //
                }
                return updateMotion();
            }
        default:
            {
                return updateMotion();
            }
    }
}

bool Hero::motionValid(const std::unique_ptr<MotionNode> &motionPtr) const
{
    if(true
            && motionPtr
            && motionPtr->type >= MOTION_BEGIN
            && motionPtr->type <  MOTION_END

            && motionPtr->direction >= DIR_BEGIN
            && motionPtr->direction <  DIR_END

            && m_processRun
            && m_processRun->onMap(m_processRun->mapID(), motionPtr->x,    motionPtr->y)
            && m_processRun->onMap(m_processRun->mapID(), motionPtr->endX, motionPtr->endY)

            && motionPtr->speed >= SYS_MINSPEED
            && motionPtr->speed <= SYS_MAXSPEED

            && motionPtr->frame >= 0
            && motionPtr->frame <  motionFrameCount(motionPtr->type, motionPtr->direction)){

        const auto nLDistance2 = mathf::LDistance2(motionPtr->x, motionPtr->y, motionPtr->endX, motionPtr->endY);
        switch(motionPtr->type){
            case MOTION_STAND:
                {
                    return !OnHorse() && (nLDistance2 == 0);
                }
            case MOTION_SPELL0:
            case MOTION_SPELL1:
                {
                    return !OnHorse() && (nLDistance2 == 0);
                }
            case MOTION_ARROWATTACK:
            case MOTION_HOLD:
            case MOTION_PUSHBACK:
            case MOTION_PUSHBACKFLY:
                {
                    return false;
                }
            case MOTION_ATTACKMODE:
                {
                    return !OnHorse() && (nLDistance2 == 0);
                }
            case MOTION_CUT:
                {
                    return false;
                }
            case MOTION_ONEVSWING:
            case MOTION_TWOVSWING:
            case MOTION_ONEHSWING:
            case MOTION_TWOHSWING:
            case MOTION_SPEARVSWING:
            case MOTION_SPEARHSWING:
            case MOTION_HITTED:
            case MOTION_WHEELWIND:
            case MOTION_RANDSWING:
                {
                    return !OnHorse() && (nLDistance2 == 0);
                }
            case MOTION_BACKDROPKICK:
                {
                    return false;
                }
            case MOTION_DIE:
                {
                    return !OnHorse() && (nLDistance2 == 0);
                }
            case MOTION_ONHORSEDIE:
                {
                    return  OnHorse() && (nLDistance2 == 0);
                }
            case MOTION_WALK:
                {
                    return !OnHorse() && (nLDistance2 == 1 || nLDistance2 == 2);
                }
            case MOTION_RUN:
                {
                    return !OnHorse() && (nLDistance2 == 4 || nLDistance2 == 8);
                }
            case MOTION_MOODEPO:
            case MOTION_ROLL:
            case MOTION_FISHSTAND:
            case MOTION_FISHHAND:
            case MOTION_FISHTHROW:
            case MOTION_FISHPULL:
                {
                    return false;
                }
            case MOTION_ONHORSESTAND:
                {
                    return OnHorse() && (nLDistance2 == 0);
                }
            case MOTION_ONHORSEWALK:
                {
                    return OnHorse() && (nLDistance2 == 1 || nLDistance2 == 2);
                }
            case MOTION_ONHORSERUN:
                {
                    return OnHorse() && (nLDistance2 == 9 || nLDistance2 == 18);
                }
            case MOTION_ONHORSEHITTED:
                {
                    return OnHorse() && (nLDistance2 == 0);
                }
            default:
                {
                    break;
                }
        }
    }

    return false;
}

bool Hero::parseAction(const ActionNode &action)
{
    m_lastActive = SDL_GetTicks();

    m_currMotion->speed = SYS_MAXSPEED;
    m_motionQueue.clear();

    const int endX   = m_forceMotionQueue.empty() ? m_currMotion->endX      : m_forceMotionQueue.back()->endX;
    const int endY   = m_forceMotionQueue.empty() ? m_currMotion->endY      : m_forceMotionQueue.back()->endY;
    const int endDir = m_forceMotionQueue.empty() ? m_currMotion->direction : m_forceMotionQueue.back()->direction;

    // 1. prepare before parsing action
    //    additional movement added if necessary but in rush
    switch(action.type){
        case ACTION_DIE:
            {
                // take this as cirtical
                // server use ReportStand() to do location sync
                for(auto &motion: makeWalkMotionQueue(endX, endY, action.x, action.y, SYS_MAXSPEED)){
                    m_forceMotionQueue.insert(m_forceMotionQueue.end(), std::move(motion));
                }
                break;
            }
        case ACTION_MOVE:
        case ACTION_STAND:
        case ACTION_SPELL:
        case ACTION_ATTACK:
            {
                m_motionQueue = makeWalkMotionQueue(endX, endY, action.x, action.y, SYS_MAXSPEED);
                break;
            }
        case ACTION_SPACEMOVE2:
            {
                break;
            }
        default:
            {
                break;
            }
    }

    // 2. parse the action
    //    now motion list is at the right grid
    switch(action.type){
        case ACTION_STAND:
            {
                m_motionQueue.push_back(std::unique_ptr<MotionNode>(new MotionNode
                {
                    .type = [this]()
                    {
                        if(OnHorse()){
                            return MOTION_ONHORSESTAND;
                        }
                        return MOTION_STAND;
                    }(),

                    .direction = action.direction,
                    .x = action.x,
                    .y = action.y,
                }));
                break;
            }
        case ACTION_MOVE:
            {
                if(auto moveNode = makeWalkMotion(action.x, action.y, action.aimX, action.aimY, action.speed)){
                    m_motionQueue.push_back(std::move(moveNode));
                    if(action.extParam.move.pickUp){
                        if(UID() != m_processRun->getMyHeroUID()){
                            throw fflerror("invalid UID to trigger pickUp action: uid = %llu", to_llu(UID()));
                        }

                        m_motionQueue.back()->addUpdate(false, [this](MotionNode *motionPtr) -> bool
                        {
                            if(motionPtr->frame < 4){
                                return false;
                            }

                            m_processRun->requestPickUp();
                            return true;
                        });
                    }
                }
                else{
                    return false;
                }
                break;
            }
        case ACTION_SPACEMOVE2:
            {
                flushMotionPending();
                jumpLoc(action.x, action.y, m_currMotion->direction);
                addAttachMagic(std::unique_ptr<AttachMagic>(new AttachMagic(u8"瞬息移动", u8"结束", -1)));
                break;
            }
        case ACTION_SPELL:
            {
                if(auto &mr = DBCOM_MAGICRECORD(action.extParam.spell.magicID)){
                    if(auto &gfxEntry = mr.getGfxEntry(u8"启动")){
                        const auto motionSpell = [&gfxEntry]() -> int
                        {
                            switch(gfxEntry.motion){
                                case 0  : return MOTION_SPELL0;
                                case 1  : return MOTION_SPELL1;
                                default : return MOTION_NONE;
                            }
                        }();

                        if(motionSpell != MOTION_NONE){
                            const auto fnGetSpellDir = [this](int nX0, int nY0, int nX1, int nY1) -> int
                            {
                                switch(mathf::LDistance2(nX0, nY0, nX1, nY1)){
                                    case 0:
                                        {
                                            return m_currMotion->direction;
                                        }
                                    default:
                                        {
                                            return PathFind::GetDirection(nX0, nY0, nX1, nY1);
                                        }
                                }
                            };

                            const auto standDir = [&fnGetSpellDir, &action, this]() -> int
                            {
                                if(action.aimUID){
                                    if(auto coPtr = m_processRun->findUID(action.aimUID)){
                                        return fnGetSpellDir(action.x, action.y, coPtr->x(), coPtr->y());
                                    }
                                }
                                else if(m_processRun->canMove(true, 0, action.aimX, action.aimY)){
                                    return fnGetSpellDir(action.x, action.y, action.aimX, action.aimY);
                                }
                                return DIR_NONE;
                            }();

                            if(directionValid(standDir)){
                                m_motionQueue.push_back(std::unique_ptr<MotionNode>(new MotionNode
                                {
                                    .type = motionSpell,
                                    .direction = standDir,
                                    .x = action.x,
                                    .y = action.y,
                                    .extParam
                                    {
                                        .spell
                                        {
                                            .magicID = action.extParam.spell.magicID,
                                        },
                                    },
                                }));

                                m_motionQueue.back()->extParam.spell.effect = std::unique_ptr<MagicSpellEffect>([&action, this]() -> MagicSpellEffect *
                                {
                                    switch(action.extParam.spell.magicID){
                                        case DBCOM_MAGICID(u8"灵魂火符"): return new TaoFireFigureEffect(m_motionQueue.back().get());
                                        default                         : return new MagicSpellEffect   (m_motionQueue.back().get());
                                    }
                                }());

                                switch(action.extParam.spell.magicID){
                                    case DBCOM_MAGICID(u8"灵魂火符"):
                                        {
                                            m_motionQueue.back()->addUpdate(true, [targetUID = action.aimUID, this](MotionNode *motionPtr) -> bool
                                            {
                                                // usually when reaches this cb the current motion is motionPtr
                                                // but not true if in flushMotionPending()

                                                if(motionPtr->frame < 3){
                                                    return false;
                                                }

                                                const auto fromX = motionPtr->x * SYS_MAPGRIDXP;
                                                const auto fromY = motionPtr->y * SYS_MAPGRIDYP;
                                                m_processRun->addFollowUIDMagic(std::unique_ptr<FollowUIDMagic>(new TaoFireFigure_RUN
                                                {
                                                    fromX,
                                                    fromY,

                                                    [fromX, fromY, targetUID, this]() -> int
                                                    {
                                                        if(const auto coPtr = m_processRun->findUID(targetUID)){
                                                            const auto [targetPX, targetPY] = coPtr->getTargetBox().center();
                                                            return pathf::getDir16(targetPX - fromX, targetPY - fromY);
                                                        }
                                                        else if(m_processRun->getMyHeroUID() == UID()){
                                                            const auto [  viewX,   viewY] = m_processRun->getViewShift();
                                                            const auto [mousePX, mousePY] = SDLDeviceHelper::getMousePLoc();
                                                            return pathf::getDir16(mousePX + viewX - fromX, mousePY + viewY - fromY);
                                                        }
                                                        else{
                                                            // not myHero
                                                            // use hero's current direction to create magic
                                                            return (m_currMotion->direction - DIR_BEGIN) * 2;
                                                        }
                                                    }(),

                                                    targetUID,
                                                    m_processRun,

                                                }))->addOnDone([targetUID, this](MagicBase *)
                                                {
                                                    if(auto coPtr = m_processRun->findUID(targetUID)){
                                                        coPtr->addAttachMagic(std::unique_ptr<AttachMagic>(new AttachMagic(u8"灵魂火符", u8"结束")));
                                                    }
                                                });
                                                return true;
                                            });
                                            break;
                                        }
                                    default:
                                        {
                                            break;
                                        }
                                }

                                if(motionSpell == MOTION_SPELL0){
                                    for(int i = 0; i < 2; ++i){
                                        m_motionQueue.push_back(std::unique_ptr<MotionNode>(new MotionNode
                                        {
                                            .type = MOTION_ATTACKMODE,
                                            .direction = standDir,
                                            .x = action.x,
                                            .y = action.y,
                                        }));
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }
        case ACTION_ATTACK:
            {
                if(auto coPtr = m_processRun->findUID(action.aimUID)){
                    if(const auto attackDir = PathFind::GetDirection(action.x, action.y, coPtr->x(), coPtr->y()); attackDir >= DIR_BEGIN && attackDir < DIR_END){
                        m_motionQueue.push_back(std::unique_ptr<MotionNode>(new MotionNode
                        {
                            .type = MOTION_ONEVSWING,
                            .direction = attackDir,
                            .x = action.x,
                            .y = action.y,
                        }));

                        m_motionQueue.push_back(std::unique_ptr<MotionNode>(new MotionNode
                        {
                            .type = MOTION_ATTACKMODE,
                            .direction = attackDir,
                            .x = action.x,
                            .y = action.y,
                        }));
                    }
                }
                else{
                    return false;
                }

                break;
            }
        case ACTION_HITTED:
            {
                m_motionQueue.push_front(std::unique_ptr<MotionNode>(new MotionNode
                {
                    .type = [this]() -> int
                    {
                        if(OnHorse()){
                            return MOTION_ONHORSEHITTED;
                        }
                        return MOTION_HITTED;
                    }(),

                    .direction = endDir,
                    .x = endX,
                    .y = endY,
                }));

                m_motionQueue.front()->addUpdate(true, [this](MotionNode *) -> bool
                {
                    for(auto &p: m_attachMagicList){
                        if(p->magicID() == DBCOM_MAGICID(u8"魔法盾")){
                            // here we replace the old 魔法盾 with a new one
                            // it's possible the old one haven't trigger its onDone callback yet, but doesn't matter
                            p.reset(new AttachMagic(u8"魔法盾", u8"挨打"));
                            p->addOnDone([this](MagicBase *)
                            {
                                // don't use p, the m_attachMagicList may re-allocate
                                // don't replace the ptr holding by p, just add a new magic, since the callback is by ptr holding by p
                                for(auto &p: m_attachMagicList){
                                    if(p->magicID() == DBCOM_MAGICID(u8"魔法盾")){
                                        addAttachMagic(std::unique_ptr<AttachMagic>(new AttachMagic(u8"魔法盾", u8"运行")));
                                        break;
                                    }
                                }
                            });
                            break;
                        }
                    }
                    return true;
                });
                break;
            }
        case ACTION_DIE:
            {
                const auto [dieX, dieY, dieDir] = motionEndPLoc(END_FORCED);
                m_forceMotionQueue.push_back(std::unique_ptr<MotionNode>(new MotionNode
                {
                    .type = [this]() -> int
                    {
                        if(OnHorse()){
                            return MOTION_ONHORSEDIE;
                        }
                        return MOTION_DIE;
                    }(),

                    .direction = dieDir,
                    .x = dieX,
                    .y = dieY,
                }));
                break;
            }
        default:
            {
                return false;
            }
    }

    // 3. after action parse
    //    verify the whole motion queue
    return motionQueueValid();
}

std::tuple<int, int> Hero::location() const
{
    if(!motionValid(m_currMotion)){
        throw fflerror("current motion is invalid");
    }

    switch(m_currMotion->type){
        case MOTION_WALK:
        case MOTION_RUN:
        case MOTION_ONHORSEWALK:
        case MOTION_ONHORSERUN:
            {
                const auto nX0 = m_currMotion->x;
                const auto nY0 = m_currMotion->y;
                const auto nX1 = m_currMotion->endX;
                const auto nY1 = m_currMotion->endY;

                const auto frameCountMoving = motionFrameCount(m_currMotion->type, m_currMotion->direction);
                if(frameCountMoving <= 0){
                    throw fflerror("invalid player moving frame count: %d", frameCountMoving);
                }

                return
                {
                    (m_currMotion->frame < (frameCountMoving / 2)) ? nX0 : nX1,
                    (m_currMotion->frame < (frameCountMoving / 2)) ? nY0 : nY1,
                };
            }
        default:
            {
                return {m_currMotion->x, m_currMotion->y};
            }
    }
}

FrameSeq Hero::motionFrameSeq(int motion, int direction) const
{
    if(!(motion >= MOTION_BEGIN && motion < MOTION_END)){
        return {};
    }

    if(!(direction >= DIR_BEGIN && direction < DIR_END)){
        return {};
    }

    switch(motion){
        case MOTION_STAND		    : return {.count =  4};
        case MOTION_ARROWATTACK		: return {.count =  6};
        case MOTION_SPELL0		    : return {.count =  5};
        case MOTION_SPELL1		    : return {.count =  5};
        case MOTION_HOLD		    : return {.count =  1};
        case MOTION_PUSHBACK		: return {.count =  1};
        case MOTION_PUSHBACKFLY		: return {.count =  1};
        case MOTION_ATTACKMODE		: return {.count =  3};
        case MOTION_CUT		        : return {.count =  2};
        case MOTION_ONEVSWING		: return {.count =  6};
        case MOTION_TWOVSWING		: return {.count =  6};
        case MOTION_ONEHSWING		: return {.count =  6};
        case MOTION_TWOHSWING		: return {.count =  6};
        case MOTION_SPEARVSWING     : return {.count =  6};
        case MOTION_SPEARHSWING     : return {.count =  6};
        case MOTION_HITTED          : return {.count =  3};
        case MOTION_WHEELWIND       : return {.count = 10};
        case MOTION_RANDSWING       : return {.count = 10};
        case MOTION_BACKDROPKICK    : return {.count = 10};
        case MOTION_DIE             : return {.count = 10};
        case MOTION_ONHORSEDIE      : return {.count = 10};
        case MOTION_WALK            : return {.count =  6};
        case MOTION_RUN             : return {.count =  6};
        case MOTION_MOODEPO         : return {.count =  6};
        case MOTION_ROLL            : return {.count = 10};
        case MOTION_FISHSTAND       : return {.count =  4};
        case MOTION_FISHHAND        : return {.count =  3};
        case MOTION_FISHTHROW       : return {.count =  8};
        case MOTION_FISHPULL        : return {.count =  8};
        case MOTION_ONHORSESTAND    : return {.count =  4};
        case MOTION_ONHORSEWALK     : return {.count =  6};
        case MOTION_ONHORSERUN      : return {.count =  6};
        case MOTION_ONHORSEHITTED   : return {.count =  3};
        default                     : return {};
    }
}

bool Hero::moving()
{
    return false
        || m_currMotion->type == MOTION_RUN
        || m_currMotion->type == MOTION_WALK
        || m_currMotion->type == MOTION_ONHORSERUN
        || m_currMotion->type == MOTION_ONHORSEWALK;
}

int Hero::WeaponOrder(int nMotion, int nDirection, int nFrame)
{
    // for player there are 33 motions, each motion has 8 directions
    // and each directions has 10 frames at most:
    //
    //      table_size = 33 * 8 * 10 = 2640
    //
    // each line of the table is for one motion at one frame
    // each eight lines stands for one motion
    //
    // and this table use gfx index rather than motion index
    // I need to convert nMotion -> nGfxMotionID before get the table item

    constexpr static int s_WeaponOrder[2640]
    {
        #include "weaponorder.inc"
    };

    const auto nGfxMotionID = gfxMotionID(nMotion);
    if(nGfxMotionID < 0){
        return -1;
    }
    return s_WeaponOrder[nGfxMotionID * 80 + (nDirection - DIR_BEGIN) * 10 + nFrame];
}

std::unique_ptr<MotionNode> Hero::makeWalkMotion(int nX0, int nY0, int nX1, int nY1, int nSpeed) const
{
    if(true
            && m_processRun
            && m_processRun->canMove(true, 0, nX0, nY0)
            && m_processRun->canMove(true, 0, nX1, nY1)

            && nSpeed >= SYS_MINSPEED
            && nSpeed <= SYS_MAXSPEED){

        static const int nDirV[][3] = {
            {DIR_UPLEFT,   DIR_UP,   DIR_UPRIGHT  },
            {DIR_LEFT,     DIR_NONE, DIR_RIGHT    },
            {DIR_DOWNLEFT, DIR_DOWN, DIR_DOWNRIGHT}};

        int nSDX = 1 + (nX1 > nX0) - (nX1 < nX0);
        int nSDY = 1 + (nY1 > nY0) - (nY1 < nY0);

        int nMotion = MOTION_NONE;
        switch(mathf::LDistance2(nX0, nY0, nX1, nY1)){
            case 1:
            case 2:
                {
                    nMotion = (OnHorse() ? MOTION_ONHORSEWALK : MOTION_WALK);
                    break;
                }
            case 4:
            case 8:
                {
                    nMotion = MOTION_RUN;
                    break;
                }
            case  9:
            case 18:
                {
                    nMotion = MOTION_ONHORSERUN;
                    break;
                }
            default:
                {
                    return {};
                }
        }

        return std::unique_ptr<MotionNode>(new MotionNode
        {
            .type = nMotion,
            .direction = nDirV[nSDY][nSDX],
            .speed = nSpeed,
            .x = nX0,
            .y = nY0,
            .endX = nX1,
            .endY = nY1,
        });
    }

    return {};
}

int Hero::GfxWeaponID(int nWeapon, int nMotion, int nDirection) const
{
    static_assert(sizeof(int) > 2, "GfxWeaponID() overflows because of sizeof(int) too small");
    if(true
            && (nWeapon    >= WEAPON_BEGIN && nWeapon    < WEAPON_END)
            && (nMotion    >= MOTION_BEGIN && nMotion    < MOTION_END)
            && (nDirection >= DIR_BEGIN    && nDirection < DIR_END   )){
        if(const auto nGfxMotionID = gfxMotionID(nMotion); nGfxMotionID >= 0){
            return ((nWeapon - WEAPON_BEGIN) << 9) + (nGfxMotionID << 3) + (nDirection - DIR_BEGIN);
        }
    }
    return -1;
}

int Hero::GfxHairID(int nHair, int nMotion, int nDirection) const
{
    static_assert(sizeof(int) > 2, "GfxHairID() overflows because of sizeof(int) too small");
    if(true
            && (nHair      >= HAIR_BEGIN   && nHair      < HAIR_END  )
            && (nMotion    >= MOTION_BEGIN && nMotion    < MOTION_END)
            && (nDirection >= DIR_BEGIN    && nDirection < DIR_END   )){
        if(const auto nGfxMotionID = gfxMotionID(nMotion); nGfxMotionID >= 0){
            return ((nHair - HAIR_BEGIN /* hair gfx id start from 0 */) << 9) + (nGfxMotionID << 3) + (nDirection - DIR_BEGIN);
        }
    }
    return -1;
}

int Hero::GfxDressID(int nDress, int nMotion, int nDirection) const
{
    static_assert(sizeof(int) > 2, "GfxDressID() overflows because of sizeof(int) too small");
    if(true
            && (nDress     >= DRESS_NONE   && nDress     < DRESS_END )  // support DRESS_NONE as naked
            && (nMotion    >= MOTION_BEGIN && nMotion    < MOTION_END)
            && (nDirection >= DIR_BEGIN    && nDirection < DIR_END   )){
        if(const auto nGfxMotionID = gfxMotionID(nMotion); nGfxMotionID >= 0){
            return ((nDress - DRESS_NONE) << 9) + (nGfxMotionID << 3) + (nDirection - DIR_BEGIN);
        }
    }
    return -1;
}

int Hero::gfxHelmetID(int nHelmet, int nMotion, int nDirection) const
{
    static_assert(sizeof(int) > 2, "gfxHelmetID() overflows because of sizeof(int) too small");
    if(true
            && (nHelmet    >= HELMET_BEGIN && nHelmet    < HELMET_END)
            && (nMotion    >= MOTION_BEGIN && nMotion    < MOTION_END)
            && (nDirection >= DIR_BEGIN    && nDirection < DIR_END   )){
        if(const auto nGfxMotionID = gfxMotionID(nMotion); nGfxMotionID >= 0){
            return ((nHelmet - HELMET_BEGIN) << 9) + (nGfxMotionID << 3) + (nDirection - DIR_BEGIN);
        }
    }
    return -1;
}

int Hero::currStep() const
{
    motionValidEx(m_currMotion);
    switch(m_currMotion->type){
        case MOTION_WALK:
        case MOTION_ONHORSEWALK:
            {
                return 1;
            }
        case MOTION_RUN:
            {
                return 2;
            }
        case MOTION_ONHORSERUN:
            {
                return 3;
            }
        default:
            {
                return 0;
            }
    }
}

ClientCreature::TargetBox Hero::getTargetBox() const
{
    switch(currMotion()->type){
        case MOTION_DIE:
            {
                return {};
            }
        default:
            {
                break;
            }
    }

    const auto nDress     = getWLItem(WLG_DRESS).itemID;
    const auto nGender    = gender();
    const auto nMotion    = currMotion()->type;
    const auto nDirection = currMotion()->direction;

    const auto texBaseID = GfxDressID(nDress, nMotion, nDirection);
    if(texBaseID < 0){
        return {};
    }

    const uint32_t texID = ((to_u32(nGender ? 1 : 0)) << 22) + ((to_u32(texBaseID & 0X01FFFF)) << 5) + currMotion()->frame;

    int dx = 0;
    int dy = 0;
    auto bodyFrameTexPtr = g_heroDB->Retrieve(texID, &dx, &dy);

    if(!bodyFrameTexPtr){
        return {};
    }

    const auto [bodyFrameW, bodyFrameH] = SDLDeviceHelper::getTextureSize(bodyFrameTexPtr);

    const auto [shiftX, shiftY] = getShift(m_currMotion->frame);
    const int startX = m_currMotion->x * SYS_MAPGRIDXP + shiftX + dx;
    const int startY = m_currMotion->y * SYS_MAPGRIDYP + shiftY + dy;

    return getTargetBoxHelper(startX, startY, bodyFrameW, bodyFrameH);
}

const SDItem &Hero::getWLItem(int wltype) const
{
    if(!(wltype >= WLG_BEGIN && wltype < WLG_END)){
        throw fflerror("invalid wltype: %d", wltype);
    }
    return m_sdWLDesp.wear.getWLItem(wltype);
}

bool Hero::setWLItem(int wltype, SDItem item)
{
    if(!(wltype >= WLG_BEGIN && wltype < WLG_END)){
        throw fflerror("invalid wear/look type: %d", wltype);
    }

    if(item.itemID == 0){
        m_sdWLDesp.wear.setWLItem(wltype, {});
        return true;
    }

    if(!item){
        throw fflerror("invalid itemID: %llu", to_llu(item.itemID));
    }

    const auto &ir = DBCOM_ITEMRECORD(item.itemID);
    if(!ir){
        throw fflerror("invalid itemID: %llu", to_llu(item.itemID));
    }

    switch(wltype){
        case WLG_DRESS:
            {
                if((to_u8sv(ir.type) != u8"衣服") || (getClothGender(item.itemID) != gender())){
                    return false;
                }
                break;
            }
        default:
            {
                if(!ir.wearable(wltype)){
                    return false;
                }
                break;
            }
    }

    m_sdWLDesp.wear.setWLItem(wltype, std::move(item));
    return true;
}

void Hero::jumpLoc(int x, int y, int direction)
{
    m_currMotion.reset(new MotionNode
    {
        .type = [this]()
        {
            if(OnHorse()){
                return MOTION_ONHORSESTAND;
            }
            return MOTION_STAND;
        }(),

        .direction = direction,
        .x = x,
        .y = y,
    });
}

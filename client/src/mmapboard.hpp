/*
 * =====================================================================================
 *
 *       Filename: mmapboard.hpp
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

#pragma once
#include "widget.hpp"
#include "tritexbutton.hpp"

class ProcessRun;
class MMapBoard: public Widget
{
    private:
        bool m_alphaOn  = false;
        bool m_extended = false;

    private:
        ProcessRun *m_processRun;

    private:
        TritexButton m_buttonAlpha;
        TritexButton m_buttonExtend;

    public:
        MMapBoard(ProcessRun *, Widget * = nullptr, bool = false);

    public:
        void drawEx(int, int, int, int, int, int) const override;

    public:
        bool processEvent(const SDL_Event &, bool) override;

    public:
        void setLoc();
        void flipExtended();
        void flipMmapShow();
        SDL_Texture *getMmapTexture() const;

    private:
        void drawFrame() const;
        void drawMmapTexture() const;

    private:
        int getFrameSize() const;
};

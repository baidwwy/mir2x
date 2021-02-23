/*
 * =====================================================================================
 *
 *       Filename: processnew.hpp
 *        Created: 08/14/2015 02:47:30 PM
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

#include <cstdint>
#include "process.hpp"
#include "labelboard.hpp"
#include "inputline.hpp"
#include "passwordbox.hpp"
#include "tritexbutton.hpp"

class ProcessNew: public Process
{
    private:
        constexpr static int m_x = 180;
        constexpr static int m_y = 145;

    private:
        LabelBoard m_LBID;
        LabelBoard m_LBPwd;
        LabelBoard m_LBPwdConfirm;

    private:
        InputLine   m_boxID;
        PasswordBox m_boxPwd;
        PasswordBox m_boxPwdConfirm;

    private:
        LabelBoard m_LBCheckID;
        LabelBoard m_LBCheckPwd;
        LabelBoard m_LBCheckPwdConfirm;

    private:
        TritexButton m_submit;
        TritexButton m_quit;

    public:
        ProcessNew();
        virtual ~ProcessNew() = default;

    public:
        int ID() const
        {
            return PROCESSID_NEW;
        }

    public:
        void update(double) override;
        void draw() override;
        void processEvent(const SDL_Event &) override;

    private:
        void doPostAccount();
        void doExit();

    private:
        // post account operation to server
        // server should respond with SMAccount for the post
        // operation : 0 : validate only
        //             1 : create
        //             2 : login
        void postAccount(const char *, const char *, int);

    private:
        bool localCheckID (const char *) const;
        bool localCheckPwd(const char *) const;
};

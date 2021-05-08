/*
 * =====================================================================================
 *
 *       Filename: clienttaodog.cpp
 *        Created: 08/31/2015 08:26:19
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

#include "fflerror.hpp"
#include "processrun.hpp"
#include "clienttaodog.hpp"

ClientTaoDog::ClientTaoDog(uint64_t uid, ProcessRun *proc, const ActionNode &action)
    : ClientMonster(uid, proc, action)
{
    fflassert(isMonster(u8"神兽"));
    switch(action.type){
        case ACTION_SPAWN:
            {
                m_currMotion.reset(new MotionNode
                {
                    .type = MOTION_MON_APPEAR,
                    .direction = directionValid(action.direction) ? to_d(action.direction) : DIR_UP,
                    .x = action.x,
                    .y = action.y,
                });

                m_standMode = false;
                break;
            }
        case ACTION_STAND:
            {
                m_currMotion.reset(new MotionNode
                {
                    .type = MOTION_MON_STAND,
                    .direction = directionValid(action.direction) ? to_d(action.direction) : DIR_UP,
                    .x = action.x,
                    .y = action.y,
                });

                m_standMode = action.extParam.stand.dog.standMode;
                break;
            }
        case ACTION_DIE:
            {
                m_currMotion.reset(new MotionNode
                {
                    .type = MOTION_MON_DIE,
                    .direction = directionValid(action.direction) ? to_d(action.direction) : DIR_UP,
                    .x = action.x,
                    .y = action.y,
                });

                m_standMode = action.extParam.die.dog.standMode;
                break;
            }
        case ACTION_ATTACK:
            {
                m_currMotion.reset(new MotionNode
                {
                    .type = MOTION_MON_DIE,
                    .direction = m_processRun->getAimDirection(action, DIR_UP),
                    .x = action.x,
                    .y = action.y,
                });

                m_standMode = action.extParam.die.dog.standMode;
                break;
            }
        default:
            {
                throw bad_reach();
            }
    }
}

void ClientTaoDog::addActionTransf(bool /* standMode */)
{
    const auto [x, y, dir] = motionEndPLoc(END_CURRENT);
    m_forceMotionQueue.push_front(std::unique_ptr<MotionNode>(new MotionNode
    {
        .type = MOTION_MON_APPEAR,
        .direction = dir,
        .x = x,
        .y = y,
    }));

    m_forceMotionQueue.front()->addUpdate(true, [this](MotionNode *) -> bool
    {
        m_standMode = true;
        return true;
    });
}

bool ClientTaoDog::onActionSpawn(const ActionNode &action)
{
    if(!m_forceMotionQueue.empty()){
        throw fflerror("found motion before spawn: %s", uidf::getUIDString(UID()).c_str());
    }

    m_currMotion.reset(new MotionNode
    {
        .type = MOTION_MON_APPEAR,
        .direction = [&action]() -> int
        {
            if(directionValid(action.direction)){
                return action.direction;
            }
            return DIR_UP;
        }(),

        .x = action.x,
        .y = action.y,
    });
    return true;
}

bool ClientTaoDog::onActionTransf(const ActionNode &action)
{
    const auto standReq = (bool)(action.extParam.transf.dog.standModeReq);
    if(m_standMode == standReq){
        return true;
    }

    // change shape immediately
    // don't wait otherwise there may has transf in the forced motion queue

    addActionTransf(m_standMode);
    return true;
}

bool ClientTaoDog::onActionAttack(const ActionNode &action)
{
    if(!m_standMode){
        bool hasTransf = false;
        for(const auto &motionPtr: m_forceMotionQueue){
            if(motionPtr->type == MOTION_MON_APPEAR){
                hasTransf = true;
            }
        }

        if(!hasTransf){
            addActionTransf(true);
        }
    }

    const auto [endX, endY, endDir] = motionEndPLoc(END_FORCED);
    m_motionQueue = makeWalkMotionQueue(endX, endY, action.x, action.y, SYS_MAXSPEED);
    if(auto coPtr = m_processRun->findUID(action.aimUID)){
        m_motionQueue.push_back(std::unique_ptr<MotionNode>(new MotionNode
        {
            .type = MOTION_MON_ATTACK0,
            .direction = [&action, endDir, coPtr]() -> int
            {
                const auto nX = coPtr->x();
                const auto nY = coPtr->y();
                if(mathf::LDistance2<int>(nX, nY, action.x, action.y) == 0){
                    return endDir;
                }
                return PathFind::GetDirection(action.x, action.y, nX, nY);
            }(),
            .x = action.x,
            .y = action.y,
        }));

        m_motionQueue.back()->addUpdate(false, [this](MotionNode *motionPtr) -> bool
        {
            if(motionPtr->frame < 5){
                return false;
            }

            if(!m_standMode){
                throw fflerror("ClientTaoDog attacks while not standing");
            }

            m_processRun->addFixedLocMagic(std::unique_ptr<FixedLocMagic>(new FixedLocMagic
            {
                u8"神兽-喷火",
                u8"运行",
                currMotion()->x,
                currMotion()->y,
                currMotion()->direction - DIR_BEGIN,
            }));
            return true;
        });
        return true;
    }
    return false;
}

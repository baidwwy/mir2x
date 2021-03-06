/*
 * =====================================================================================
 *
 *       Filename: ascendstr.hpp
 *        Created: 07/20/2017 00:31:01
 *    Description: decide to not implement it as magic
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
#include <cmath>

enum AscendStrType: int
{
    ASCENDSTR_MISS = 0,
    ASCENDSTR_NUM0 = 1,
    ASCENDSTR_NUM1 = 2,
    ASCENDSTR_NUM2 = 3,
};

class AscendStr
{
    private:
        int m_type;
        int m_value;

    private:
        int m_x;
        int m_y;

    private:
        double m_tick;

    public:
        AscendStr(int, int, int, int);

    public:
        ~AscendStr() = default;

    public:
        void Draw(int, int);
        void Update(double);

    private:
        int Type () const { return m_type;  }
        int Value() const { return m_value; }

    private:
        int X() const
        {
            return m_x + to_d(std::lround(Ratio() * 50.0));
        }

        int Y() const
        {
            return m_y - to_d(std::lround(Ratio() * 50.0));
        }

    private:
        double Tick() const
        {
            return m_tick;
        }

    public:
        double Ratio() const
        {
            return Tick() / 3000.0;
        }
};

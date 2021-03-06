/*
  This file is part of Allie Chess.
  Copyright (C) 2018, 2019 Adam Treat

  Allie Chess is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Allie Chess is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Allie Chess.  If not, see <http://www.gnu.org/licenses/>.

  Additional permission under GNU GPL version 3 section 7
*/

#include "clock.h"

#include <QDebug>
#include <QtMath>

#include "options.h"

using namespace Chess;

Clock::Clock(QObject *parent)
    : QObject(parent),
      m_isActive(false),
      m_whiteTime(-1),
      m_whiteIncrement(-1),
      m_blackTime(-1),
      m_blackIncrement(-1),
      m_moveTime(false),
      m_infinite(false),
      m_deadline(0),
      m_trendFactor(0),
      m_materialScore(0),
      m_halfMoveNumber(0)
{
    m_timeout = new QTimer(this);
    m_timeout->setTimerType(Qt::PreciseTimer);
    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, this, &Clock::timeout);
}

Clock::~Clock()
{
}

qint64 Clock::time(Chess::Army army) const
{
    if (army == White)
        return m_whiteTime;
    else
        return m_blackTime;
}

void Clock::setTime(Chess::Army army, qint64 time)
{
    if (army == White)
        m_whiteTime = time;
    else
        m_blackTime = time;
}

qint64 Clock::increment(Chess::Army army) const
{
    if (army == White)
        return m_whiteIncrement;
    else
        return m_blackIncrement;
}

void Clock::setIncrement(Chess::Army army, qint64 inc)
{
    if (army == White)
        m_whiteIncrement = inc;
    else
        m_blackIncrement = inc;
}

void Clock::setInfinite(bool infinite)
{
    m_infinite = infinite;
}

void Clock::setMoveTime(qint64 time)
{
    m_moveTime = time;
}

void Clock::startDeadline(Chess::Army army)
{
    m_isActive = true;
    m_info = SearchInfo();
    m_onTheClock = army;
    m_timer.restart();
    m_timeout->stop();
    calculateDeadline(false /*isPartial*/);
}

void Clock::updateDeadline(const SearchInfo &info, bool isPartial)
{
    m_info = info;
    calculateDeadline(isPartial);
}

qint64 Clock::elapsed() const
{
    return m_timer.nsecsElapsed() / 1000000;
}

bool Clock::hasExpired() const
{
    return m_timer.hasExpired(m_deadline);
}

qint64 Clock::timeToDeadline() const
{
    if (m_infinite)
        return -1;
    return m_deadline - elapsed();
}

qint64 Clock::trendFactor() const
{
    return m_trendFactor;
}

bool Clock::lessThanMoveOverhead() const
{
    return timeToDeadline() < Options::globalInstance()->option("MoveOverhead").value().toInt();
}

bool Clock::pastMoveOverhead() const
{
    return elapsed() > Options::globalInstance()->option("MoveOverhead").value().toInt();
}

int Clock::expectedHalfMovesTillEOG() const
{
    // Heuristic from http://facta.junis.ni.ac.rs/acar/acar200901/acar2009-07.pdf
    if (m_materialScore < 20)
        return m_materialScore + 10;
    else if (20 <= m_materialScore && m_materialScore <= 60)
        return qRound((3/8 * float(m_materialScore))) + 22;
    else
        return qRound((5/4 * float(m_materialScore))) - 30;
}

float easingCurve(float x)
{
#if !defined(EXPERIMENTAL)
    return x;
#else
    // Meant to provide a slight easing so that most time is spent in middle game
    return (float(qSin(M_PI * qreal(x))) * 0.5f) + 0.5f;
#endif
}

void Clock::calculateDeadline(bool isPartial)
{
    Q_UNUSED(isPartial);
    if (m_infinite) {
        m_deadline = -1;
        m_timeout->stop();
        return;
    }

    const int minimumDepth = 3;
    const qint64 overhead = Options::globalInstance()->option("MoveOverhead").value().toInt();
    const qint64 t = time(m_onTheClock);
    const qint64 inc = increment(m_onTheClock);
    const qint64 maximum = t - overhead;
    const qint64 ideal = qRound(easingCurve(t / expectedHalfMovesTillEOG() + inc));

    // Largest factor is a quarter of remaining time
    qint64 trendFactor = qRound((maximum / 4) * m_info.trendDegree);
    if (m_info.trend != Better)
        m_trendFactor += trendFactor;
    else
        m_trendFactor = m_trendFactor / 2;
    m_trendFactor = qMax(qint64(0), m_trendFactor);

    // Calculate the actual deadline
    qint64 deadline = 5000;
    if (m_moveTime != -1)
        deadline = m_moveTime - overhead;
    else if (t != -1 && m_info.depth >= minimumDepth)
        deadline = qMin(maximum, ideal /*+ m_trendFactor*/);
    else if (t != -1)
        deadline = maximum;
    m_deadline = qMax(qint64(0), deadline);
    m_timeout->start(qMax(int(0), int(m_deadline - elapsed())));
}

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

#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <QVector>

#include "game.h"

class Zobrist {
public:
    static Zobrist *globalInstance();
    quint64 hash(const Game::Position &position) const;

private:
    Zobrist();
    quint64 m_pieceKeys[64][12];
    QVector<quint64> m_otherKeys;
    friend class MyZobrist;
};

#endif

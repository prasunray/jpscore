/**
 * \file        NeighborhoodSearch.h
 * \date        Nov 16, 2010
 * \version     v0.7
 * \copyright   <2009-2015> Forschungszentrum J?lich GmbH. All rights reserved.
 *
 * \section License
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 **/
#pragma once

#include "Grid2D.hpp"
#include "IteratorPair.hpp"
#include "NeighborhoodIterator.hpp"
#include "geometry/Point.hpp"

class NeighborhoodSearch
{
    double _cellSize;
    Grid2D<Pedestrian*> _grid{};

public:
    explicit NeighborhoodSearch(double cellSize);
    ~NeighborhoodSearch();
    NeighborhoodSearch(const NeighborhoodSearch&) = default;
    NeighborhoodSearch(NeighborhoodSearch&&) = default;
    NeighborhoodSearch& operator=(const NeighborhoodSearch&) = default;
    NeighborhoodSearch& operator=(NeighborhoodSearch&&) = default;

    /**
     *Update the cells occupation
     */
    void Update(const std::vector<std::unique_ptr<Pedestrian>>& peds);

    IteratorPair<NeighborhoodIterator, NeighborhoodEndIterator>
    GetNeighboringAgents(Point pos, double radius) const;
};

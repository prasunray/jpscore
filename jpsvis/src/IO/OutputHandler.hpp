/**
 * \file        OutputHandler.h
 * \date        Nov 20, 2010
 * \version     v0.6
 * \copyright   <2009-2014> Forschungszentrum Jülich GmbH. All rights reserved.
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
 * \section Description
 *
 *
 **/
#pragma once

#include "../general/Macros.hpp"

#include <fstream>
#include <iostream>
#include <vector>

class OutputHandler
{
protected:
    int _nWarnings;
    int _nErrors;

public:
    OutputHandler()
    {
        _nWarnings = 0;
        _nErrors = 0;
    };
    virtual ~OutputHandler(){};

    int GetWarnings();
    void incrementWarnings();
    int GetErrors();
    void incrementErrors();
    void ProgressBar(double TotalPeds, double NowPeds);

    virtual void Write(const std::string& str);
    virtual void Write(const char* string, ...);
};

class STDIOHandler : public OutputHandler
{
public:
    void Write(const std::string& str);
};

class FileHandler : public OutputHandler
{
private:
    std::ofstream _pfp;

public:
    FileHandler(const char* fn);
    virtual ~FileHandler();
    void Write(const std::string& str);
    void Write(const char* string, ...);
};

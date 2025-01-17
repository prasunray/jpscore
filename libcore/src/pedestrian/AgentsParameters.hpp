/**
 * \file        AgentsParameters.h
 * \date        Jul 4, 2014
 * \version     v0.7
 * \copyright   <2009-2015> Forschungszentrum Jülich GmbH. All rights reserved.
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
 * This class contains the different force models dependent parameters for the agents.
 * They are used to defined different population group, for instance children, men, elderly...
 * It is done by specifying different sizes, desired velocities and reaction times
 *
 **/
#pragma once

#include <random>

class AgentsParameters
{
public:
    /**
     * Constructor
     */
    AgentsParameters(int id, int seed = 1234);

    /**
     * Destructor
     */
    virtual ~AgentsParameters();

    /**
     * @return the ID of the agents parameters sets.
     */
    int GetID();

    /**
     * Set the ID of the parameter set
     * @param id
     */
    void SetID(int id); // not implemented

    /**
     * Initialize the desired velocity distribution
     * @param mean, mean value
     * @param stv, standard deviation
     */
    void InitV0(double mean, double stv);

    /**
     * Initialize the desired velocity distribution walking up stairs
     * @param mean, mean value
     * @param stv, standard deviation
     * @param smoothFactor, smooth factor for transition from v0 to v0Up and vice versa
     */
    void InitV0UpStairs(double mean, double stv, double smoothFactor);

    /**
     * Initialize the desired velocity distribution walking downstairs
     * @param mean, mean value
     * @param stv, standard deviation
     * @param smoothFactor, smooth factor for transition from v0 to v0Up and vice versa
     */
    void InitV0DownStairs(double mean, double stv, double smoothFactor);

    /**
     * Initialize the speed distribution of escalators upstairs
     * @param mean, mean value
     * @param stv, standard deviation
     * @param smoothFactor, smooth factor for transition from v0 to v0Up or vice versa
     */
    void InitEscalatorUpStairs(double mean, double stv, double smoothFactor);

    /**
     * Initialize the speed distribution of escalators downstairs
     * @param mean, mean value
     * @param stv, standard deviation
     * @param smoothFactor, smooth factor for transition from v0 to v0Up or vice versa
     */
    void InitEscalatorDownStairs(double mean, double stv, double smoothFactor);

    /**
     * Initialize the maximal value if the major axis
     * @param mean, mean value
     * @param stv, standard deviation
     */
    void InitBmax(double mean, double stv);

    /**
     * Initialize the minimal value if the major axis
     * @param mean, mean value
     * @param stv, standard deviation
     */
    void InitBmin(double mean, double stv);

    /**
     * Initialize the minimal value of the minor axis
     * @param mean, mean value
     * @param stv, standard deviation
     */
    void InitAmin(double mean, double stv);

    /**
     * Initialize the reaction time
     * @param mean, mean value
     * @param stv, standard deviation
     */
    void InitAtau(double mean, double stv);

    /**
     * Initialize the reaction time
     * @param mean, mean value
     * @param stv, standard deviation
     */
    void InitTau(double mean, double stv);

    /**
     * Initialize the reaction time
     * @param mean, mean value
     * @param stv, standard deviation
     */
    void InitT(double mean, double stv);

    void EnableStretch(bool stretch);

    /**
     * @return a random number following the distribution
     */
    double GetV0();

    /**
     * @return a random number following the distribution
     */
    double GetV0UpStairs();

    /**
     * @return a random number following the distribution
     */
    double GetV0DownStairs();

    /**
     * @return a random number following the distribution
     */
    double GetEscalatorUpStairs();

    /**
     * @return a random number following the distribution
     */
    double GetEscalatorDownStairs();

    /**
     * @return a random number following the distribution
     */
    double GetBmax();

    /**
     * @return a random number following the distribution
     */
    double GetBmin();

    /**
     * @return a random number following the distribution
     */
    double GetAtau();

    /**
     * @return a random number following the distribution
     */
    double GetAmin();

    /**
     * @return a random number following the distribution
     */
    double GetTau();

    /**
     * @return a random number following the distribution
     */
    double GetT();

    /**
     * @return whether Ellipse stretching is enabled
     */
    bool StretchEnabled();

    /**
     * return a summary of the parameters
     */
    std::string writeParameter();

    double GetAminMean();

    double GetBmaxMean();

private:
    int _id;
    std::default_random_engine _generator;
    std::normal_distribution<double> _V0;
    std::normal_distribution<double> _V0UpStairs;
    std::normal_distribution<double> _V0DownStairs;
    std::normal_distribution<double> _EscalatorUpStairs;
    std::normal_distribution<double> _EscalatorDownStairs;
    std::normal_distribution<double> _V0IdleEscalatorUpStairs;
    std::normal_distribution<double> _V0IdleEscalatorDownStairs;
    std::normal_distribution<double> _Bmax;
    std::normal_distribution<double> _Bmin;
    bool _enableStretch = true;
    std::normal_distribution<double> _Atau;
    std::normal_distribution<double> _Amin;
    std::normal_distribution<double> _Tau;
    std::normal_distribution<double> _T;
    const double judge = 10000;

public:
    /**
     * this is the constant c in f and g in
     * \include ../../../docs/pages/jpscore/jpscore_desired_speed.md
     * function for up- and downstairs is assumed to be symmetrical.
     */
    double _smoothFactorUpStairs;
    double _smoothFactorDownStairs;
    double _smoothFactorEscalatorUpStairs;
    double _smoothFactorEscalatorDownStairs;
    double _smoothFactorIdleEscalatorUpStairs;
    double _smoothFactorIdleEscalatorDownStairs;
};

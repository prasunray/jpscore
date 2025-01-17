#pragma once

#include "Simulation.hpp"
#include "events/Event.hpp"

#include <chrono>

std::chrono::nanoseconds EventMinTime(const Event& event);

void ProcessEvent(const CreatePedestrianEvent& event, Simulation& sim);
void ProcessEvent(const DoorEvent& event, Simulation& sim);
void ProcessEvent(const TrainEvent& event, Simulation& sim);

#include <iostream>
#include <iomanip>
#include "./movement.h"
using namespace std;


Movement::Movement(int regularMoves, int alliedRoadMoves, int nonAlliedRoadMoves, int forestMoves, int seaMoves, int barkMoves) : regularMoves(regularMoves),
alliedRoadMoves(alliedRoadMoves), nonAlliedRoadMoves(nonAlliedRoadMoves), forestMoves(forestMoves), seaMoves(seaMoves), barkMoves(barkMoves) {
}


// Getter for regularMoves
int Movement::getRegular() const {
	return regularMoves;
}

// Getter for alliedRoadMoves
int Movement::getAlliedRoad() const {
	return alliedRoadMoves;
}

// Getter for nonAlliedRoadMoves
int Movement::getNonAlliedRoad() const {
	return nonAlliedRoadMoves;
}

// Getter for forestMoves
int Movement::getForest() const {
	return forestMoves;
}

// Getter for seaMoves
int Movement::getSea() const {
	return seaMoves;
}

// Getter for barkMoves
int Movement::getBark() const {
	return barkMoves;
}

// Getter for totalCost
float Movement::getCost() const {
	return totalCost;
}


float Movement::calculateMovement() {
	regular = regularMoves * 1.0;

	alliedRoad = alliedRoadMoves * 0.75;
	nonAlliedRoad = nonAlliedRoadMoves * 1.00;

	forest = forestMoves * 2.0;
	sea = seaMoves * 0.5;
	bark = barkMoves * 1.5;

	totalCost = regular + alliedRoad + nonAlliedRoad + forest + sea + bark;

	return totalCost;
}

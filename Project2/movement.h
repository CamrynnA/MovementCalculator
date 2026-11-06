#ifndef MOVEMENT_H
#define MOVEMENT_H


class Movement {

public:

	Movement(int regularMoves, int alliedRoadMoves, int nonAlliedRoadMoves, int forestMoves, int seaMoves, int barkMoves);

	// Used by calculateMovement()
	float regular;
	float alliedRoad;
	float nonAlliedRoad;
	float forest;
	float sea;
	float bark; // embark / disembark

	// Getters
	int getRegular() const;
	int getAlliedRoad() const;
	int getNonAlliedRoad() const;
	int getForest() const;
	int getSea() const;
	int getBark() const;

	// Gets the cost calculated by calculateMovement
	float getCost() const;

	// Holds the total calculated by calculateMovement
	float totalCost;

	//functions
	float calculateMovement();


private:

	// Returned by getters
	int regularMoves;
	int alliedRoadMoves;
	int nonAlliedRoadMoves;
	int forestMoves;
	int seaMoves;
	int barkMoves;

};



#endif
#pragma once

/* The MIT License (MIT)
 * 
 * Copyright (c) 2015 Giovanni Ortolani, Taneli Mikkonen, Pingjiang Li, Tommi Puolamaa, Mitra Vahida
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. */

#include "Grid.h"
#include "Common.h"

Grid::Grid(unsigned int size, const Ogre::Matrix3 face)
{
	value = allocate2DArray<int>(size, size);
	memset(value[0], 0, sizeof(int)*size*size);

	gridSize = size;
	orientation = face;

	xplusNeighbour = NULL;
	xminusNeighbour = NULL;
	yplusNeighbour = NULL;
	yminusNeighbour = NULL;
}

Grid::~Grid()
{
	free2DArray(value);
}

unsigned int Grid::getSize()
{
	return gridSize;
}

Ogre::Matrix3 Grid::getOrientation()
{
	return orientation;
}

void Grid::setValue(unsigned int x, unsigned y, int val)
{
	value[y][x] = val;
}

int Grid::getValue(unsigned int x, unsigned int y)
{
	return value[y][x];
}

/* Return vector for a normalized (radius 1.0) sphere */
Ogre::Vector3 Grid::projectToSphere(unsigned int x, unsigned int y)
{
	Ogre::Vector3 pos;
	float mSizeFloat, xFloat, yFloat, halfStep;

	mSizeFloat = static_cast<float>(gridSize);
	xFloat = static_cast<float>(x)/mSizeFloat;
	yFloat = static_cast<float>(y)/mSizeFloat;
	halfStep = 1.0f/(mSizeFloat*2.0f);

	// For convenience treat xy-heightmap as a xz-plane in sphere-coordinates
	pos.x = 1.0f-halfStep - xFloat*2.0f;
	pos.z = -1.0f+halfStep + yFloat*2.0f;
	pos.y = 1.0f;
	// Simple re-orientation
	pos = orientation*pos;
	// project heightMap to sphere
	pos.normalise();

	return pos;
}

/* Function to set neighboring HeightMaps */
void Grid::setNeighbours(Grid *xPlus, Grid *xMinus, Grid *yPlus, Grid *yMinus)
{
	xplusNeighbour = xPlus;
	xminusNeighbour = xMinus;
	yplusNeighbour = yPlus;
	yminusNeighbour = yMinus;
}

/* Get pointer for neighboring grid by using an enumerator */
Grid *Grid::getNeighbourPtr(Grid_neighbour neighbour)
{
	if (neighbour == neighbour_XP)
		return xplusNeighbour;

	else if (neighbour == neighbour_XM)
		return xminusNeighbour;

	else if (neighbour == neighbour_YP)
		return yplusNeighbour;

	else if (neighbour == neighbour_YM)
		return yminusNeighbour;
	else
		return NULL;
}

/* When entry_x or entry_y, which are used to point positions on 2D-grids,
 * is on grid-boundary and switch to neighboring HeightMap is desired,
 * this can be used to output new values of entry_x and entry_y for referenced
 * HeightMap.
 * Return values:
 *	On success, returns new entry_x, entry_y and function return value true.
 *	On failure, entry_x, entry_y are not modified and function return value is false. */
bool Grid::getNeighbourEntryCoordinates(Grid_neighbour neighbour
											 , unsigned int &entry_x, unsigned int &entry_y)
{
	Grid *gridNeighbour;

	// Is entry coordinates at the edge of map
	if ( !( (entry_x == this->getSize()-1) || (entry_x == 0)
			|| (entry_y == this->getSize()-1) || entry_y == 0 ) )
		return false;

	gridNeighbour = this->getNeighbourPtr(neighbour);

	if (gridNeighbour == NULL)
		return false;

	if (this->gridSize != gridNeighbour->getSize())
	{
		std::cerr << "Grid and neighbour sizes differ!" << std::endl;
		return false;
	}

	/* Assuming both square-grids face their frontface in same direction,
	 * there are 16 different neighboring combinations. */

	// If current HeightMap is x-minus neighbour for neighbour of interest
	if (this == gridNeighbour->getNeighbourPtr(Grid::neighbour_XM))
	{
		// Check all 4 side-orientations for a square.
		if (neighbour == neighbour_XP)
		{
			entry_x = 0;
		}
		else if (neighbour == neighbour_XM)
		{
			/* There is no usecase for this combination in the current code. */
			std::cerr << "This should not happen! XM and Neighbour XM" << std::endl;
			return false;
		}
		else if (neighbour == neighbour_YP)
		{
			entry_y = gridNeighbour->getSize() - entry_x - 1;
			entry_x = 0;
		}
		else if (neighbour == neighbour_YM)
		{
			entry_y = entry_x;
			entry_x = 0;
		}
	}
	else if (this == gridNeighbour->getNeighbourPtr(Grid::neighbour_XP))
	{
		if (neighbour == neighbour_XP)
		{
			/* There is no usecase for this combination in the current code. */
			std::cerr << "This should not happen! XP and Neighbour XP" << std::endl;
			return false;
		}
		else if (neighbour == neighbour_XM)
		{
			entry_x = gridNeighbour->getSize()-1;
		}
		else if (neighbour == neighbour_YP)
		{
			entry_y = entry_x;
			entry_x = gridNeighbour->getSize()-1;
		}
		else if (neighbour == neighbour_YM)
		{
			entry_y = gridNeighbour->getSize()-entry_x-1;
			entry_x = gridNeighbour->getSize()-1;
		}
	}
	else if (this == gridNeighbour->getNeighbourPtr(Grid::neighbour_YM))
	{
		if (neighbour == neighbour_XP)
		{
			entry_x = gridNeighbour->getSize() - entry_y -1;
			entry_y = 0;
		}
		else if (neighbour == neighbour_XM)
		{
			entry_x = entry_y;
			entry_y = 0;
		}
		else if (neighbour == neighbour_YP)
		{
			entry_y = 0;
		}
		else if (neighbour == neighbour_YM)
		{
			entry_x = gridNeighbour->getSize() - entry_x -1;
			entry_y = 0;
		}
	}
	else if (this == gridNeighbour->getNeighbourPtr(Grid::neighbour_YP))
	{
		if (neighbour == neighbour_XP)
		{
			entry_x = entry_y;
			entry_y = gridNeighbour->getSize()-1;
		}
		else if (neighbour == neighbour_XM)
		{
			entry_x = gridNeighbour->getSize() - entry_y - 1;
			entry_y = gridNeighbour->getSize() - 1;
		}
		else if (neighbour == neighbour_YP)
		{
			entry_x = gridNeighbour->getSize()-entry_x-1;
			entry_y = gridNeighbour->getSize()-1;
		}
		else if (neighbour == neighbour_YM)
		{
			entry_y = gridNeighbour->getSize() - 1;
		}
	}
	else
	{
		std::cerr << "Neighbouring grid does not have this Grid as a neighbour"
				  << std::endl;
		return false;
	}

	return true;
}

// CrystalLighting.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// C++11
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <set>
#include <string>
#include <fstream>

using namespace std;

vector< int > move_row { 1, -1, 0, 0 };
vector< int > move_col { 0, 0, 1, -1 };

class Crystal
{
public:
    Crystal()
    {}

    Crystal( int R, int C, char Color ) :
        r { R }, c { C }, color { Color - '0' }
    {}

public:
    int r, c;
    int color;
    map< int, bool > lights;
};

class Cell
{
public:
    Cell()
    {}

    Cell( int R, int C ) :
        r { R }, c { C }
    {}

public:
    int r, c;
    vector< int > crystals;
};

class CrystalLighting
{
public:
    bool isInside( int r, int c ) 
    {
        return ( r >= 0 && r < H && c >= 0 && c < W );
    }

    bool isEmptyCell( int r, int c )
    {
        if ( isInside( r, c ) && board[ r ][ c ] == '.' )
            return true;

        return false;
    }

    void addCells( int r, int c )
    {
        for ( int dir = 0; dir < 4; ++dir )
        {
            int cell_r = r + move_row[ dir ];
            int cell_c = c + move_col[ dir ];
            while ( isEmptyCell( cell_r, cell_c ) )
            {
                int key = cell_r * W + cell_c;
                if ( emptyCells.count( key ) == 0 )
                    emptyCells[ key ] = Cell( cell_r, cell_c );

                emptyCells[ key ].crystals.push_back( r * W + c );
                cell_r += move_row[ dir ];
                cell_c += move_col[ dir ];
            }
        }
    }

    void getCrystals()
    {
        for ( int r = 0; r < H; ++r )
            for ( int c = 0; c < W; ++c )
                if ( board[ r ][ c ] != '.' && board[ r ][ c ] != 'X' )
                {
                    crystals[ r * W + c ] = Crystal( r, c, board[ r ][ c ] );
                    addCells( r, c );
                }
    }

    vector< string > placeItems( vector< string > targetBoard, int costLantern, int costMirror, int costObstacle, int maxMirrors, int maxObstacles )
    {
        board = targetBoard;
        H = targetBoard.size();
        W = targetBoard[ 0 ].size();

        getCrystals();

        return { "0 7 2", "9 5 1" };
    }
private:
    map< int, Crystal > crystals;
    map< int, Cell > emptyCells;
    //vector< Cell > emptyCells;
    vector< string > board;
    int H, W;
};
// -------8<------- end of solution submitted to the website -------8<-------

void readTestFromFile()
{
    ifstream infile( "../input.txt" );
    int costLantern, costMirror, costObstacle, maxMirrors, maxObstacles;
    infile >> costLantern >> costMirror >> costObstacle >> maxMirrors >> maxObstacles;
    vector< string > board;
    while ( !infile.eof() )
    {
        string s;
        infile >> s;
        board.push_back( s );
    }

    CrystalLighting cl;
    vector<string> ret = cl.placeItems( board, costLantern, costMirror, costObstacle, maxMirrors, maxObstacles );

}

template<class T> void getVector( vector<T>& v ) {
    for ( int i = 0; i < v.size(); ++i )
        cin >> v[ i ];
}

int main() {

    readTestFromFile();

    //CrystalLighting cl;
    //int H;
    //cin >> H;
    //vector<string> targetBoard( H );
    //getVector( targetBoard );
    //int costLantern, costMirror, costObstacle, maxMirrors, maxObstacles;
    //cin >> costLantern >> costMirror >> costObstacle >> maxMirrors >> maxObstacles;

    //vector<string> ret = cl.placeItems( targetBoard, costLantern, costMirror, costObstacle, maxMirrors, maxObstacles );
    //cout << ret.size() << endl;
    //for ( int i = 0; i < (int)ret.size(); ++i )
    //    cout << ret[ i ] << endl;
    //cout.flush();
}


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
#include <list>
#include <set>
#include <string>
#include <fstream>
#include <random>

using namespace std;

std::random_device rd;  //Will be used to obtain a seed for the random number engine
std::mt19937 gen( 1 ); //Standard mersenne_twister_engine seeded with rd()

const double MOVE_PROB = 0.5;
const double CONFLICT_PENALTY = -10000;


vector< int > move_row { 1, -1, 0, 0 };
vector< int > move_col { 0, 0, 1, -1 };

double sigmoid( double x )
{
    return 1 / ( 1 + exp( -x ) );
}

struct RowCol
{
    RowCol() {};
    RowCol( int R, int C ) : r { R }, c { C } {};
    RowCol( const RowCol & RC ) : r { RC.r }, c { RC.c } {};
    int r, c;
};

class Crystal
{
public:
    Crystal()
    {}

    Crystal( RowCol RC, char Color ) :
        rc { RC }, color { Color - '0' }
    {}

    int potentialScore()
    {
        if ( color == 1 || color == 2 || color == 4 )
            return 20;

        return 30;
    }

    int actualScore()
    {
        int sum = 0;
        for ( auto & l : lights )
            if ( l.second )
                sum += l.first;

        if ( sum == color )
            return potentialScore();
        else if ( sum == 0 )
            return 0;
        else if (
            ( color == 3 && ( sum == 1 || sum == 2 ) ) ||
            ( color == 5 && ( sum == 2 || sum == 3 ) ) ||
            ( color == 6 && ( sum == 4 || sum == 2 ) ) )
            return -5;
        else
            return -10;
    }

    void reset()
    {
        for ( auto & l : lights )
            l.second = false;
    }

public:
    // location of the crystal
    RowCol rc;

    // the actual color of the crystal
    int color;

    // this map should have 3 elements max each corresponding to one of three possible colors
    // indicating which lanterns shine on this crystal
    map< int, bool > lights;
};

class Cell
{
public:
    Cell()
    {}

    Cell( RowCol RC ) :
        rc { RC }, potential { 0 }
    {}

    //bool isLit()
    //{
    //    if ( lanterns.size() == 0 )
    //        return false;

    //    return true;
    //}

public:
    // location of the cell
    RowCol rc;

    // list of crystals that are lightable from this cell
    vector< int > crystals;

    // list of all empty cells that are lightable from this cell
    vector< int > litableCells;

    // list of lanterns that light this cell
    //list < int > lanterns;

    int potential;
};

class Lantern
{
public:
    Lantern()
    {}

    Lantern( RowCol RC, int Color ) :
        rc { RC }, color { Color }, score { 0 }
    {}

public:
    RowCol rc;
    int color;
    double score;
};

class CrystalLighting
{
public:
    bool isInside( int r, int c ) 
    {
        return ( r >= 0 && r < m_H && c >= 0 && c < m_W );
    }

    bool isEmptyCell( int r, int c )
    {
        if ( isInside( r, c ) && m_board[ r ][ c ] == '.' )
            return true;

        return false;
    }

    // finds all empty cells that are reachable from a cell with location (r,c)
    vector< RowCol > findReachableCells( int r, int c )
    {
        vector< RowCol > ret;
        for ( int dir = 0; dir < 4; ++dir )
        {
            int cell_r = r + move_row[ dir ];
            int cell_c = c + move_col[ dir ];
            while ( isEmptyCell( cell_r, cell_c ) )
            {
                ret.push_back( RowCol( cell_r, cell_c ) );
                cell_r += move_row[ dir ];
                cell_c += move_col[ dir ];
            }
        }

        return ret;
    }

    // find all crystals and create a list of all empty cells from which these crystals can be lit
    void getCrystals()
    {
        for ( int r = 0; r < m_H; ++r )
            for ( int c = 0; c < m_W; ++c )
                if ( m_board[ r ][ c ] != '.' && m_board[ r ][ c ] != 'X' )
                {
                    m_crystals[ r * m_W + c ] = Crystal( RowCol( r, c ), m_board[ r ][ c ] );

                    // find all cells that can lit this crystal
                    vector< RowCol > cells = findReachableCells( r, c );

                    for ( auto & rc : cells )
                    {
                        int key = rc.r * m_W + rc.c;
                        // if this empty cell has not been added to the emptyCells map
                        // add the cell and record all cells that are lightable from this cell
                        if ( m_emptyCells.count( key ) == 0 )
                        {
                            m_emptyCells[ key ] = Cell( rc );
                            vector< RowCol > rcells = findReachableCells( rc.r, rc.c );
                            for ( auto & x : rcells )
                                m_emptyCells[ key ].litableCells.push_back( x.r * m_W + x.c );
                        }

                        // record that a crystal at ( r, c ) is lightable from this cell
                        m_emptyCells[ key ].crystals.push_back( r * m_W + c );
                        m_emptyCells[ key ].potential += m_crystals[ r * m_W + c ].potentialScore();
                    }
                }
    }

    // pick a random color based on the colors of crystals lightable from this cell
    int pickRandomColor( int cell_key )
    {
        Cell x = m_emptyCells[ cell_key ];

        // pick a color on random
        vector< int > colors;
        for ( auto & key : x.crystals )
        {
            switch ( m_crystals[ key ].color )
            {
            case 3:
                colors.push_back( 1 );
                colors.push_back( 2 );
                break;
            case 5:
                colors.push_back( 1 );
                colors.push_back( 4 );
                break;
            case 6:
                colors.push_back( 2 );
                colors.push_back( 4 );
                break;
            default:
                colors.push_back( m_crystals[ key ].color );
            }
        }

        std::uniform_int_distribution<> dis_color( 0, colors.size() - 1 );
        return colors[ dis_color( gen ) ];
    }

    // put a lanter into a random empty cell
    // this placement should not create conflicts
    void placeRandomLantern()
    {
        // get a list of all cells that are valid for lantern placement
        vector< int > unlitCells;
        for ( auto & cell : m_emptyCells )
            if ( m_litCells.count( cell.second.rc.r * m_W + cell.second.rc.c ) == 0 )
                unlitCells.push_back( cell.first );

        // pick a cell on random
        std::uniform_int_distribution<> dis_pos( 0, unlitCells.size() - 1 );
        int cell_key = unlitCells[ dis_pos( gen ) ];
        Cell x = m_emptyCells[ cell_key ];
        
        int clr = pickRandomColor( cell_key );

        // create a lantern
        m_lanterns.push_back( Lantern( x.rc, clr ) );

        // lite the crystals with a chosen color
        for ( auto & key : x.crystals )
            m_crystals[ key ].lights[ clr ] = true;

        // lite the lightable cells so they become not valid for lantern placement on the next step
        // including itself
        m_litCells[ cell_key ].push_back( cell_key );
        for ( auto & key : m_emptyCells[ cell_key ].litableCells )
            m_litCells[ key ].push_back( cell_key );
    }

    // calculate local lantern scores in a current solution
    void calculateLanternScores()
    {
        for ( auto & lantern : m_lanterns )
        {
            int key = lantern.rc.r * m_W + lantern.rc.c;
            double potential_score = m_emptyCells[ key ].potential;
            double actual_score = 0;

            for ( auto & i : m_emptyCells[ key ].crystals )
                actual_score += m_crystals[ i ].actualScore();

            lantern.score = sigmoid( 2 * actual_score / potential_score );
        }
    }

    // move and change color of lanterns in the current solution
    list< Lantern > alterLanterns( map< int, vector< int > > & litCells )
    {
        //calculateLanternScores();
        list< Lantern > altered;

        std::uniform_real_distribution<> dis( 0, 1 );
        for ( auto const & lantern : m_lanterns )
        {
            RowCol rc = lantern.rc;
            int color = lantern.color;
            // coin toss whether to alter this lantern
            double u = dis( gen );
            if ( u < ( 1 - lantern.score ) )
            {
                // coin toss whether to move this lantern 
                u = dis( gen );
                if ( u < MOVE_PROB )
                {
                    vector< RowCol > candidates;
                    int key = lantern.rc.r * m_W + lantern.rc.c;
                    // go over all cells reachable from this cells and save those that are not more than 4 cells away
                    for ( auto & k : m_emptyCells[ key ].litableCells )
                        if ( ( abs( m_emptyCells[ k ].rc.r - lantern.rc.r ) +
                            abs( m_emptyCells[ k ].rc.c - lantern.rc.c ) ) <= 4 )
                            candidates.push_back( m_emptyCells[ k ].rc );
                    
                    std::uniform_int_distribution<> dis_keys( 0, candidates.size() - 1 );
                    rc = candidates[ dis_keys( gen ) ];
                }

                color = pickRandomColor( rc.r * m_W + rc.c );
            }

            altered.push_back( Lantern( rc, color ) );

            int lantern_key = rc.r * m_W + rc.c;
            litCells[ lantern_key ].push_back( lantern_key );
            for ( auto & key : m_emptyCells[ lantern_key ].litableCells )
                litCells[ key ].push_back( lantern_key );
        }

        return altered;
    }

    int calculateSolutionScore( const list< Lantern > & lanterns, map< int, vector< int > > & litCells )
    {
        // reset crystals
        for ( auto & crystal : m_crystals )
            crystal.second.reset();

        // light up crystals given lanterns
        int penalty = 0;
        for ( auto & lantern : lanterns )
        {
            int key = lantern.rc.r * m_W + lantern.rc.c;
            for ( auto & k : m_emptyCells[ key ].crystals )
                m_crystals[ k ].lights[ lantern.color ] = true;

            if ( litCells[ key ].size() > 1 )
                penalty += CONFLICT_PENALTY;
        }

        int score = 0;
        for ( auto & crystal : m_crystals )
            score += crystal.second.actualScore();

        score += penalty;

        return score;
    }

    void optimize()
    {
        for ( int i = 0; i < 20; ++i )
        {
            // this will light up the crystals with the current m_lanterns
            int current_sol_score = calculateSolutionScore( m_lanterns, m_litCells );

            // calculate local lantern score for lantern alterations
            calculateLanternScores();

            map< int, vector< int > > alteredLitCells;
            list< Lantern > alteredLanterns = alterLanterns( alteredLitCells );

            int new_sol_score = calculateSolutionScore( alteredLanterns, alteredLitCells );

            if ( new_sol_score > current_sol_score )
            {
                m_lanterns = alteredLanterns;
                m_litCells = alteredLitCells;
            }
        }
    }

    vector< string > placeItems( vector< string > targetBoard, int costLantern, int costMirror, int costObstacle, int maxMirrors, int maxObstacles )
    {
        m_board = targetBoard;
        m_H = targetBoard.size();
        m_W = targetBoard[ 0 ].size();

        getCrystals();

        for ( int i = 0; i < 10; ++i )
            placeRandomLantern();

        optimize();

        stringstream ss;
        vector< string > output;
        for ( auto & l : m_lanterns )
        {
            ss << l.rc.r << " " << l.rc.c << " " << l.color;
            output.push_back( ss.str() );
            ss.str( "" );
            ss.clear();
        }

        return output;
    }
private:
    map< int, Crystal > m_crystals;
    map< int, Cell > m_emptyCells;
    map< int, vector< int > > m_litCells;
    list< Lantern > m_lanterns;
    vector< string > m_board;
    int m_H, m_W;
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

    //readTestFromFile();

    CrystalLighting cl;
    int H;
    cin >> H;
    vector<string> targetBoard( H );
    getVector( targetBoard );
    int costLantern, costMirror, costObstacle, maxMirrors, maxObstacles;
    cin >> costLantern >> costMirror >> costObstacle >> maxMirrors >> maxObstacles;

    vector<string> ret = cl.placeItems( targetBoard, costLantern, costMirror, costObstacle, maxMirrors, maxObstacles );
    cout << ret.size() << endl;
    for ( int i = 0; i < (int)ret.size(); ++i )
        cout << ret[ i ] << endl;
    cout.flush();
}


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
std::mt19937 gen( 42 ); //Standard mersenne_twister_engine seeded with rd()

const double REMOVE_PROB = 0.2;
const double CONFLICT_PENALTY = -10000;


vector< int > move_row { 1, -1, 0, 0 };
vector< int > move_col { 0, 0, 1, -1 };

double sigmoid( double x )
{
    return 1 / ( 1 + exp( -x ) );
}

struct Zone
{
    Zone() {};

    Zone( int R, int C, int H, int W ) : r { R }, c { C }, h { H }, w { W }
    {};

    int r, c;
    int h, w;
};

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
            if ( l.second.size() > 0 )
                sum += l.first;

        if ( sum == color )
            return potentialScore();
        else if ( sum == 0 )
            return 0;
        else if (
            ( color == 3 && ( sum == 1 || sum == 2 ) ) ||
            ( color == 5 && ( sum == 2 || sum == 3 ) ) ||
            ( color == 6 && ( sum == 4 || sum == 2 ) ) )
            return -10;
        else
            return -10;
    }

    void reset()
    {
        for ( auto & l : lights )
            l.second.clear();
    }

public:
    // location of the crystal
    RowCol rc;

    // the actual color of the crystal
    int color;

    // this map should have 3 elements max each corresponding to one of three possible colors
    // indicating which lanterns shine on this crystal
    map< int, vector< int > > lights;
};

class Cell
{
public:
    Cell()
    {}

    Cell( RowCol RC ) :
        rc { RC }, potential { 0 }
    {}

public:
    // location of the cell
    RowCol rc;

    // list of crystals that are lightable from this cell
    vector< int > crystals;

    // list of all empty cells that are lightable from this cell
    vector< int > litableCells;

    int potential;

    vector< int > colors;
};

class Lantern
{
public:
    Lantern()
    {}

    Lantern( RowCol RC, int Color ) :
        rc { RC }, color { Color }, score { 0 }//, on { true }
    {}

public:
    RowCol rc;
    int color;
    double score;
    //bool on;
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

    int accessWays( int r, int c )
    {
        int x = 0;

        if ( isEmptyCell( r - 1, c ) ) x++;
        if ( isEmptyCell( r, c - 1 ) ) x++;
        if ( isEmptyCell( r + 1, c ) ) x++;
        if ( isEmptyCell( r, c + 1 ) ) x++;

        return x;
    }

    // find all crystals and create a list of all empty cells from which these crystals can be lit
    void getCrystals()
    {
        for ( int r = 0; r < m_H; ++r )
            for ( int c = 0; c < m_W; ++c )
                if ( m_board[ r ][ c ] != '.' && m_board[ r ][ c ] != 'X' ) // Lantern
                {
                    int crystal_key = r * m_W + c;
                    m_crystals[ crystal_key ] = Crystal( RowCol( r, c ), m_board[ r ][ c ] );

                    int accs = accessWays( r, c );

                    if ( ( m_crystals[ crystal_key ].color == 1 || m_crystals[ crystal_key ].color == 2
                        || m_crystals[ crystal_key ].color == 4 ) && accs > 0 )
                        m_maxLanterns++;
                    else if ( accs > 1 )
                        m_maxLanterns += 2;


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
                            addCellToZonesList( rc );
                            vector< RowCol > rcells = findReachableCells( rc.r, rc.c );
                            for ( auto & x : rcells )
                                m_emptyCells[ key ].litableCells.push_back( x.r * m_W + x.c );
                        }

                        // record that a crystal at ( r, c ) is lightable from this cell
                        m_emptyCells[ key ].crystals.push_back( r * m_W + c );
                        m_emptyCells[ key ].potential += m_crystals[ r * m_W + c ].potentialScore();
                    }
                }
                else if ( m_board[ r ][ c ] == '.' ) // From this empty cell you cannot light up any lanterns, might be good for blocks...
                {
                    int key = r * m_W + c;
                    // if this empty cell has not been added to the emptyCells map
                    // add the cell and record all cells that are lightable from this cell
                    if ( m_emptyCells.count( key ) == 0 )
                    {
                        m_emptyCells[ key ] = Cell( RowCol( r, c ) );
                        addCellToZonesList( RowCol( r, c ) );
                        vector< RowCol > rcells = findReachableCells( r, c );
                        for ( auto & x : rcells )
                            m_emptyCells[ key ].litableCells.push_back( x.r * m_W + x.c );
                    }
                }
    }

    // pick a random color based on the colors of crystals lightable from this cell
    vector< int > getColors( Cell & x )
    {
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
        return colors;
    }


    // pick a random color based on the colors of crystals lightable from this cell
    int pickRandomColor( const vector< int > & colors )
    {
        std::uniform_int_distribution<> dis_color( 0, colors.size() - 1 );
        return colors[ dis_color( gen ) ];
    }

    // put a lanter into a random empty cell
    // this placement should not create conflicts
    void placeRandomLantern( list< Lantern > & lanterns, map< int, vector< int > > & litCells, int zoneInd, map< int, Crystal > & zoneCrystals )
    {
        // get a list of all cells that are valid for lantern placement
        vector< int > unlitCells;
        //for ( auto & cell : m_emptyCells )
        for ( auto & key : m_zoneKeys[ zoneInd ] )
            if ( litCells.count( key ) == 0 && m_litCells.count( key ) == 0 && m_emptyCells[ key ].potential > 0 )
                unlitCells.push_back( key );

        if ( unlitCells.size() > 0 )
        {
            // pick a cell on random
            std::uniform_int_distribution<> dis_pos( 0, unlitCells.size() - 1 );
            int cell_key = unlitCells[ dis_pos( gen ) ];
            Cell x = m_emptyCells[ cell_key ];

            int clr = pickRandomColor( x.colors );

            // create a lantern
            lanterns.push_back( Lantern( x.rc, clr ) );

            for ( auto & k : m_emptyCells[ cell_key ].crystals )
            {
                if ( zoneCrystals.count( k ) == 0 )
                    zoneCrystals[ k ] = m_crystals[ k ];

                zoneCrystals[ k ].lights[ clr ].push_back( cell_key );
            }

            // lite the lightable cells so they become not valid for lantern placement on the next step
            // including itself
            litCells[ cell_key ].push_back( cell_key );
            for ( auto & key : m_emptyCells[ cell_key ].litableCells )
                litCells[ key ].push_back( cell_key );
        }
    }

    // calculate local lantern scores in a current solution
    void calculateLanternScores( list< Lantern > & lanterns, map< int, Crystal > & zoneCrystals )
    {
        for ( auto & lantern : lanterns )
        {
            int key = lantern.rc.r * m_W + lantern.rc.c;
            double potential_score = m_emptyCells[ key ].potential;
            double actual_score = 0;

            for ( auto & i : m_emptyCells[ key ].crystals )
            {
                // penilize for lighting the same crystal with the same color from more than one lantern
                int s = zoneCrystals[ i ].lights[ lantern.color ].size();
                if ( s == 1 )
                    actual_score += m_crystals[ i ].actualScore();
                else if ( s > 1 )
                    actual_score -= 5;
            }

            lantern.score = sigmoid( 3 * actual_score / potential_score );
        }
    }

    // move and change color of lanterns in the current solution
    list< Lantern > alterLanterns( const list< Lantern > & lanterns, map< int, vector< int > > & litCells )
    {
        //calculateLanternScores();
        list< Lantern > altered;

        std::uniform_real_distribution<> dis( 0, 1 );
        for ( auto const & lantern : lanterns )
        {
            RowCol rc = lantern.rc;
            int color = lantern.color;
            bool remove = false;
            // coin toss whether to alter this lantern
            double u = dis( gen );
            if ( u < ( 1 - lantern.score ) )
            {
                // coin toss whether to delete this lantern
                u = dis( gen );
                if ( u < REMOVE_PROB )
                    remove = true;
                else
                {
                    color = pickRandomColor( m_emptyCells[ rc.r * m_W + rc.c ].colors );
                }
            }

            if ( !remove )
            {
                altered.push_back( Lantern( rc, color ) );
                int lantern_key = rc.r * m_W + rc.c;
                litCells[ lantern_key ].push_back( lantern_key );

                for ( auto & key : m_emptyCells[ lantern_key ].litableCells )
                    litCells[ key ].push_back( lantern_key );
            }
        }

        return altered;
    }

    int calculateSolutionScore( const list< Lantern > & lanterns, map< int, vector< int > > & litCells )
    {
        // reset crystals
        /*for ( auto & crystal : m_crystals )
            crystal.second.reset();

        for ( auto & lantern : m_lanterns )
        {
            int key = lantern.rc.r * m_W + lantern.rc.c;
            for ( auto & k : m_emptyCells[ key ].crystals )
                m_crystals[ k ].lights[ lantern.color ].push_back( key );
        }*/

        // light up crystals given lanterns
        int penalty = 0;
        int totalLanternCost = 0;
        map< int, Crystal > crystalsToScore;

        //vector< int > crystalsToScore;
        for ( auto & lantern : lanterns )
        {
            int key = lantern.rc.r * m_W + lantern.rc.c;
            for ( auto & k : m_emptyCells[ key ].crystals )
            {
                if ( crystalsToScore.count( k ) == 0 )
                    crystalsToScore[ k ] = m_crystals[ k ];

                crystalsToScore[ k ].lights[ lantern.color ].push_back( key );
            }

            //if ( litCells[ key ].size() > 1 )
            //    penalty += CONFLICT_PENALTY;

            totalLanternCost += m_costLantern;
        }

        int score = 0;
        for ( auto & cryst : crystalsToScore )
            score += cryst.second.actualScore();

        score += penalty;
        score -= totalLanternCost;

        return score;
    }

    void optimize()
    {
        int Tmax = 1000;
        int N = 5;
        std::uniform_real_distribution<> dis( 0, 1 );

        for ( int z = 0; z < m_zones.size(); ++z )
        {
            list< Lantern > zoneLanterns;
            map< int, vector< int > > zoneLitCells;
            double current_sol_score = -100000;
            for ( int i = 0; i < Tmax; ++i )
            {
                list< Lantern > alteredLanterns = zoneLanterns;
                map< int, vector< int > > alteredLitCells = zoneLitCells;
                map< int, Crystal > zoneCrystals;

                double v = exp( -(double)( 3 * i ) / Tmax );
                double u;
                for ( int j = 0; j < N; ++j )
                {
                    u = dis( gen );
                    if ( u < v )
                        placeRandomLantern( alteredLanterns, alteredLitCells, z, zoneCrystals );
                }

                // calculate local lantern score for lantern alterations
                calculateLanternScores( alteredLanterns, zoneCrystals );
                
                alteredLitCells.clear();
                alteredLanterns = alterLanterns( alteredLanterns, alteredLitCells );

                // this will light up the crystals with the current m_lanterns
                //double current_sol_score = calculateSolutionScore( zoneLanterns, zoneLitCells );

                double new_sol_score = calculateSolutionScore( alteredLanterns, alteredLitCells );

                double w = sigmoid( ( 30 * i ) * ( new_sol_score - current_sol_score ) / abs( current_sol_score ) / Tmax );
                u = dis( gen );
                if ( u < w )
                {
                    zoneLanterns = alteredLanterns;
                    zoneLitCells = alteredLitCells;
                    current_sol_score = new_sol_score;
                }
            }

            m_lanterns.insert( m_lanterns.end(), zoneLanterns.begin(), zoneLanterns.end() );

            for ( auto & x : zoneLitCells )
            {
                if ( m_litCells.count( x.first ) == 0 )
                    m_litCells[ x.first ] = x.second;
                else
                    m_litCells[ x.first ].insert( m_litCells[ x.first ].end(), x.second.begin(), x.second.end() );
            }

            for ( auto & crystal : m_crystals )
                crystal.second.reset();

            for ( auto & lantern : m_lanterns )
            {
                int key = lantern.rc.r * m_W + lantern.rc.c;
                for ( auto & k : m_emptyCells[ key ].crystals )
                    m_crystals[ k ].lights[ lantern.color ].push_back( key );
            }
        }
    }

    void createZones()
    {
        int d = 11;
        //int delta = d;
        int zone_H, zone_W;

        if ( m_H > d )
            zone_H = d;
        else
            zone_H = m_H;

        if ( m_W > d )
            zone_W = d;
        else
            zone_W = m_W;

        int i = 0, j = 0;

        while ( i < m_H )
        {
            int h = 0;
            if ( i + zone_H < m_H )
                h = zone_H;
            else
                h = m_H - i;

            j = 0;

            while ( j < m_W )
            {
                int w = 0;
                if ( j + zone_H < m_W )
                    w = zone_W;
                else
                    w = m_W - j;

                m_zones.push_back( Zone( i, j, h, w ) );
                j += w;
            }
            i += h;
        }

        for ( auto i = 0; i < m_zones.size(); ++i )
            m_zoneKeys.push_back( vector< int >() );
    }

    void addCellToZonesList( RowCol rc )
    {
        for ( auto i = 0; i < m_zones.size(); ++i )
        {
            if ( ( rc.r >= m_zones[ i ].r && rc.r < ( m_zones[ i ].r + m_zones[ i ].h ) )
                && ( rc.c >= m_zones[ i ].c && rc.c < ( m_zones[ i ].c + m_zones[ i ].w ) ) )
                m_zoneKeys[ i ].push_back( rc.r * m_W + rc.c );
        } 
    }

    vector< string > placeItems( vector< string > targetBoard, int costLantern, int costMirror, int costObstacle, int maxMirrors, int maxObstacles )
    {
        m_board = targetBoard;
        m_H = targetBoard.size();
        m_W = targetBoard[ 0 ].size();
        m_costLantern = costLantern;
        m_costMirror = costMirror;
        m_costObstacle = costObstacle;
        m_maxLanterns = 0;

        createZones();
        getCrystals();
        
        for ( auto & cell : m_emptyCells )
        {
            if ( cell.second.crystals.size() > 0 )
                cell.second.colors = getColors( cell.second );
        }

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
    int m_costLantern;
    int m_costMirror;
    int m_costObstacle;
    int m_maxLanterns;
    vector< Zone > m_zones;
    vector< vector< int > > m_zoneKeys;
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

   // readTestFromFile();

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


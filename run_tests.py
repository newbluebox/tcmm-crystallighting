from subprocess import Popen, PIPE
import pandas as pd
import numpy as np
import os.path

scores_file = './/scores.csv'

def evaluate_current_solution() :
    scores = []
    for i in range( 1, 101 ) :
        process = Popen( [ 'java', '-jar', 'Tester.jar', '-exec', './/x64//Release//CrystalLighting.exe', '-seed', str( i ), '-novis' ], stdout = PIPE )
        ( output, err ) = process.communicate()
        s = output.decode( 'utf-8' )
        k1 = s.rfind( '\r' )
        scores.append( float( s[ 8 : k1 ] ) )
        process.kill()
        print( i, " ", s )
    
    return np.array( scores )

def save_solution( solution_name, new_scores ) :
    df = pd.DataFrame()
    if os.path.exists( scores_file ) :
        df = pd.read_csv( scores_file )

    df[ solution_name ] = new_scores
    df.to_csv( scores_file, index = False )

def compare( s1, s2 ):
    res = ''
    #if ( s2 != -1 and s2 <= s1 ):
    #    res = 'better'
    #if ( s2 != -1 and s2 > s1 ):
    #    res = 'worse'
    #if ( s2 == -1 ):
    #    res = 'invalid'

    return s1 - s2

def compare_scores( sol1, sol2 ):
    df = pd.read_csv( scores_file )
    comp = df.apply( lambda row : compare( row[ sol1 ], row[ sol2 ] ), axis = 1 )
    comp.to_csv( 'comparison.csv', index = False )


scores = evaluate_current_solution()
#save_solution( 'test2', scores )

#compare_scores( 'last sub', 'test2' )
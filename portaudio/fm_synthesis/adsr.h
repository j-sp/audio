/************************************************************/
/* Function implementing an ADSR envelope                   */
/*                                                          */
/* Arguments:                                               */
/* t: time to evaluate function at, in seconds              */
/* attack_time: duration of attack phase                    */
/* decay_time: duration of decay phase                      */
/* sustain_time: duration of sustain phase                  */
/* sustain_level: level of sustain phase (<1)               */
/* release_time: duration of release phase                  */
/************************************************************/

double adsr(double t, double attack_time, double decay_time, double sustain_time, 
            double sustain_level, double release_time);
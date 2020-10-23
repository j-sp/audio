double adsr(double t, double attack_time, double decay_time, double sustain_time, 
            double sustain_level, double release_time) {

    if (t<=attack_time) {
        return t/attack_time;
    } else
}
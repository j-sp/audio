double adsr(double t, double attack_time, double decay_time, double sustain_time, 
            double sustain_level, double release_time) {

    if (t<=attack_time) {
        return t/attack_time;
    }
    if(t<=attack_time+decay_time) {
        return 1 - ((t-attack_time)*((1-sustain_level)/decay_time));
    }
    if(t<=attack_time+decay_time+sustain_time){
        return sustain_level;
    }
    if(t<=attack_time+decay_time+sustain_time+release_time){
        return sustain_level - ((t-(attack_time+decay_time+sustain_time))*(sustain_level/release_time));
    } else {
        return 0;
    }
}
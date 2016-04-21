
struct sbmon_accum {
	voltage U;
	current I;
	current I_request;
};


rv sbmon_new(sbon **sm);
rv sbmon_free(sbmon *sm);

rv sbmon_full_scan(sbmon *sm);


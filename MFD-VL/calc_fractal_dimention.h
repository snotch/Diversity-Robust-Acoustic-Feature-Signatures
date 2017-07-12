//
//  calc_fractal_dimention.h
//  fractalAnalysis
//
//  Created by sunouchi_mbp on 12/02/15.
//  Copyright (c) 2013. All rights reserved.
//

#ifndef fractalAnalysis_calc_fractal_dimention_h
#define fractalAnalysis_calc_fractal_dimention_h

struct sq_status {
  long long val;
  long long idx;
};

void calc_fractal_dimention(const char *, const char *);
void calc_max(long long *, long long, long long, struct sq_status *);
void calc_min(long long *, long long, long long, struct sq_status *);

#endif

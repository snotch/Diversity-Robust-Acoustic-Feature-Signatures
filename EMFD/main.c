//
//  main.c
//  fractalAnalysis
//
//  Created by sunouchi_mbp on 12/02/14.
//  Copyright (c) 2013 All rights reserved.
//

#include <stdio.h>
#include "calc_fractal_dimention.h"

int main(int argc, char *argv[]) {
  if(argc > 1) {
    if(argc == 2) {
      calc_fractal_dimention(argv[1], NULL);
    } else if(argc == 3) {
      calc_fractal_dimention(argv[1], argv[2]);
    }
  } else {
    printf("\nPlease set input filepath.\n\n");
  }
  return 0;
}


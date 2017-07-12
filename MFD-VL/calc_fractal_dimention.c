//
//  calc_fractal_dimention.c
//  fractalAnalysis
//
//  Created by motohiro, Sunouchi on 18 Sep, 2013.
//  Copyright (c) 2013. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "sox.h"
#include "util.h"
#include "calc_fractal_dimention.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define STEPS_PER_SAMPLE 1
#define RADIUS           22050
#define RADIUS_DIV_NUM   11
#define RADIUS_DIV_FACT  sqrt(2)
#define MAX_DIMENTION 2
#define MIN_DIMENTION 1

/* 2205 samples = 50ms */
#define ANALYSIS_WINDOW 2205

void calc_fractal_dimention(const char *path, const char *out_path)
{
	//uint64_t seek;
  SOX_SAMPLE_LOCALS;
  sox_format_t *ft;
	sox_sample_t *buf;
	
	long long *signal_vals;
  
  int64_t i, k, div, radius, last_radius, diameter, len;
  
  uint64_t total_area_sum, last_total_area_sum;
  uint64_t block_size, readed, counter;
  
  struct sq_status max_v, min_v;
  
  double fd;
  
  FILE *ofp = NULL;
  
	/* printf doesn't use buffer */
	setvbuf(stdout, (char *)NULL, _IONBF, 0);
  
  /*
	 * //////////////////////////////////////
	 */
	
  sox_init();
	ft = sox_open_read(path, NULL, NULL, NULL);
  
	if(ft != NULL) {
		printf("\n---- outline of input file ----\n");
		printf("filename: %s\n", ft->filename);
		printf("filetype: %s\n", ft->filetype);
		printf("sampling rate: %f\n", ft->signal.rate);
		printf("channels: %d\n", ft->signal.channels);
		printf("precision: %d\n", ft->signal.precision);
		printf("length (sample * chans): %ld\n", ft->signal.length);
		printf("encoding: %d\n", ft->encoding.encoding);
		printf("bits per sample: %d\n", ft->encoding.bits_per_sample);
		printf("compression: %f\n", ft->encoding.compression);
		printf("\n---- end ----\n\n");
	} else {
    sox_quit();
    exit(0);
  }
  
  if (ft->signal.channels != 1 && ft->signal.channels != 2) {
    printf("Channel number should be 1 or 2.\n");
    sox_close(ft);
    sox_quit();
    exit(0);
  }
  
  if (
      ft->signal.length <= ft->signal.channels * RADIUS * 2
      ) {
    printf("The length of sound should be longer than 44100 samplings.\n");
    sox_close(ft);
    sox_quit();
    exit(0);
  }
  
  if (out_path != NULL) {
    ofp = fopen(out_path, "w");
  }
  
	for (div = 0; div < RADIUS_DIV_NUM; div++) {
    total_area_sum = 0;
    len = 0;
    readed = 0;
    max_v.val = INT32_MIN;
    max_v.idx = 0;
    min_v.val = INT32_MAX;
    min_v.idx = 0;
    
    radius = (int)(RADIUS / pow(RADIUS_DIV_FACT, div) + 0.5);
    
    sox_seek(ft, 0, SOX_SEEK_SET);
    diameter = radius * 2;
    block_size = diameter * ft->signal.channels;
    buf = malloc(sizeof(sox_sample_t) * block_size);
    signal_vals = malloc(sizeof(int64_t) * diameter);
    
    //printf("radius:%lld\t", radius);
    
    if (ft->signal.channels == 1) {
      
      // phase 1 : the unit square moves to initial position
      
      readed = sox_read(ft, buf, block_size);
      if (readed < block_size) {
        printf("Couldn't read enough datas from the sound file.\n");
        free(buf);
        free(signal_vals);
        if (ofp != NULL) {
          fclose(ofp);
        }
        sox_close(ft);
        sox_quit();
        exit(0);
      }
      
      for (i = 0; i < diameter; i++) {
        signal_vals[i] = SOX_SAMPLE_TO_UNSIGNED_16BIT(buf[i], ft->clips);
      }
      
      for (i = 0; i < radius; i++) {
        if (signal_vals[i] > max_v.val) {
          max_v.val = signal_vals[i];
          max_v.idx = i;
        }
        if (signal_vals[i] < min_v.val) {
          min_v.val = signal_vals[i];
          min_v.idx = i;
        }
      }
      
      for (i = radius; i < diameter; i++) {
        total_area_sum += (max_v.val - min_v.val);
        len++;
        if (signal_vals[i] > max_v.val) {
          max_v.val = signal_vals[i];
          max_v.idx = i;
        }
        if (signal_vals[i] < min_v.val) {
          min_v.val = signal_vals[i];
          min_v.idx = i;
        }
      }
      //printf("%" PRId64 "\t", total_area_sum);
      
      // phase 2
      while((readed = sox_read(ft, buf, block_size)) && readed > 0) {
        for(i = 0; i < readed; i++) {
          total_area_sum += (max_v.val - min_v.val);
          len++;
          signal_vals[i] = SOX_SAMPLE_TO_UNSIGNED_16BIT(buf[i], ft->clips);
          
          if (max_v.idx == i) {
            calc_max(signal_vals, diameter, i, &max_v);
          } else if (signal_vals[i] > max_v.val) {
            max_v.val = signal_vals[i];
            max_v.idx = i;
          }
          
          if (min_v.idx == i) {
            calc_min(signal_vals, diameter, i, &min_v);
          } else if (signal_vals[i] < min_v.val) {
            min_v.val = signal_vals[i];
            min_v.idx = i;
          }
        }
      }
      //printf("%" PRId64 "\t", total_area_sum);
    } else if (ft->signal.channels == 2) {
      
      // phase 1 : the unit square moves to initial position
      
      readed = sox_read(ft, buf, block_size);
      if (readed < block_size) {
        printf("Couldn't read enough datas from the sound file.\n");
        free(buf);
        free(signal_vals);
        if (ofp != NULL) {
          fclose(ofp);
        }
        sox_close(ft);
        sox_quit();
        exit(0);
      }
      for (i = 0; i < diameter; i++) {
        k = i * 2;
        signal_vals[i] = SOX_SAMPLE_TO_UNSIGNED_16BIT(buf[k], ft->clips);
      }
      for (i = 0; i < radius; i++) {
        if (signal_vals[i] > max_v.val) {
          max_v.val = signal_vals[i];
          max_v.idx = i;
        }
        if (signal_vals[i] < min_v.val) {
          min_v.val = signal_vals[i];
          min_v.idx = i;
        }
      }
      for (i = radius; i < diameter; i++) {
        total_area_sum += (max_v.val - min_v.val);
        len++;
        if (signal_vals[i] > max_v.val) {
          max_v.val = signal_vals[i];
          max_v.idx = i;
        }
        if (signal_vals[i] < min_v.val) {
          min_v.val = signal_vals[i];
          min_v.idx = i;
        }
      }
      //printf("%" PRId64 "\t", total_area_sum);

      // phase 2
      while((readed = sox_read(ft, buf, block_size)) && readed > 0) {
        k = readed / 2;
        for(i = 0; i < k; i++) {
          total_area_sum += (max_v.val - min_v.val);
          len++;
          signal_vals[i] = SOX_SAMPLE_TO_UNSIGNED_16BIT(buf[i * 2], ft->clips);
          
          if (max_v.idx == i) {
            calc_max(signal_vals, diameter, i, &max_v);
          } else if (signal_vals[i] > max_v.val) {
            max_v.val = signal_vals[i];
            max_v.idx = i;
          }
          
          if (min_v.idx == i) {
            calc_min(signal_vals, diameter, i, &min_v);
          } else if (signal_vals[i] < min_v.val) {
            min_v.val = signal_vals[i];
            min_v.idx = i;
          }
        }
      }
      //printf("%" PRId64 "\t", total_area_sum);
    }
    
    //phase3
    max_v.val = INT32_MIN;
    min_v.val = INT32_MAX;
    
    counter = 0;
    
    if (readed <= 0) {
      i = diameter - 1;
    } else {
      i = readed / ft->signal.channels - 1;
    }
    while (counter < radius) {
      if (signal_vals[i] > max_v.val) {
        max_v.val = signal_vals[i];
      }
      if (signal_vals[i] < min_v.val) {
        min_v.val = signal_vals[i];
      }
      
      if (i <= 0) {
        i = diameter - 1;
      } else {
        i--;
      }
      counter++;
    }
    
    while (counter < diameter) {
      total_area_sum += (max_v.val - min_v.val);
      len++;
      if (signal_vals[i] > max_v.val) {
        max_v.val = signal_vals[i];
      }
      if (signal_vals[i] < min_v.val) {
        min_v.val = signal_vals[i];
      }
      
      if (i <= 0) {
        i = diameter - 1;
      } else {
        i--;
      }
      counter++;
    }
    
    
    //printf("%lld\t%lld\t%lld\t", total_area_sum, len, diameter);
    
    total_area_sum = total_area_sum + (len * diameter);
    //printf("%lld\t", total_area_sum);
    
    if (div > 0) {
      fd = 2.0 - log((double)last_total_area_sum / total_area_sum) / log((double)last_radius / radius);
      if(ofp != NULL) {
        fprintf(ofp, "%f", fd);
        if (div < RADIUS_DIV_NUM - 1) {
          fprintf(ofp, ",");
        } else {
          fprintf(ofp, "\n");
        }
      } else {
        printf("%f", fd);
        if (div < RADIUS_DIV_NUM - 1) {
          printf(",");
        } else {
          printf("\n");
        }
      }
    }
    last_total_area_sum = total_area_sum;
    last_radius = radius;
    
    //printf("radius:%d area:%" PRId64 " len:%d\n", radius, total_area_sum, len);
    
    free(buf);
    free(signal_vals);
    sox_close(ft);
    ft = sox_open_read(path, NULL, NULL, NULL);
  }
  
  
  if (ofp != NULL) {
    fclose(ofp);
  }
  
  printf("\n");
	sox_close(ft);
	sox_quit();
  
  exit(0);
}

void calc_max(long long vals[], long long len, long long start_idx, struct sq_status *s)
{
  long long i;
  
  s->val = INT32_MIN;
  for (i = start_idx; i >= 0; i--) {
    if (vals[i] > s->val) {
      s->val = vals[i];
      s->idx = i;
    }
  }
  for (i = len - 1; i > start_idx; i--) {
    if (vals[i] > s->val) {
      s->val = vals[i];
      s->idx = i;
    }
  }
}

void calc_min(long long vals[], long long len, long long start_idx, struct sq_status *s)
{
  long long i;
  
  s->val = INT32_MAX;
  for (i = start_idx; i >= 0; i--) {
    if (vals[i] < s->val) {
      s->val = vals[i];
      s->idx = i;
    }
  }
  for (i = len - 1; i > start_idx; i--) {
    if (vals[i] < s->val) {
      s->val = vals[i];
      s->idx = i;
    }
  }
}


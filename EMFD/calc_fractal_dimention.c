//
//  calc_fractal_dimention.c
//  fractalAnalysis
//
//  Created by motohiro, Sunouchi on 28 May, 2013.
//  Copyright (c) 2013. All rights reserved.
//
// Cut off last frame version.
// Analyze by log scale for signature.
// Output raw data (not histogram)

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "sox.h"
#include "util.h"
#include "calc_fractal_dimention.h"

#define STEPS_PER_SAMPLE 1
#define RADIUS           1
#define RADIUS_MULT_MIN  1
#define RADIUS_MULT_MAX  17
#define BIN_NUM 32
#define MAX_DIMENTION 2
#define MIN_DIMENTION 1
/* 2205 samples = 50ms */
#define ANALYSIS_WINDOW 2205

int print_bit(uint64_t v)
{
	uint64_t bit = (uint64_t)1 << 63;

	// printf("v: %llu", v);
	// printf("\t bit: %llu\t", bit);
	for(; bit != 0; bit >>= 1) {
		if(v & bit) {
			putchar('1');
		} else {
			putchar('O');
		}
	}
  return 1;
}

uint64_t bitcount(uint64_t v)
{
	unsigned int pos = 0;
	uint64_t r;

	r = v >> (sizeof(v) * CHAR_BIT - pos);
	r = r - ((r >> 1) & ~0UL / 3);
	r = (r & ~0UL / 5) + ((r >> 2) & ~0UL / 5);
	r = (r + (r >> 4)) & ~0UL / 17;
	r = (r * (~0UL / 255)) >> ((sizeof(v) - 1) * CHAR_BIT);

	return r;
}

void calc_fractal_dimention(const char *path, const char *out_path)
{
	//uint64_t seek;

  sox_format_t *ft;
	sox_sample_t *buf;
	size_t block_size, blocks, readed;

	uint16_t vals_r[ANALYSIS_WINDOW]; //, vals_l[ANALYSIS_WINDOW];
  int32_t i, tmp, len;
  double mfd_array[RADIUS_MULT_MAX - RADIUS_MULT_MIN];
  bool end_block;

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

  if (ft->signal.channels < 1 || ft->signal.channels > 2) {
    printf("Channel number should be 1 or 2.\n");
    sox_close(ft);
    sox_quit();
    exit(0);
  }

  if (out_path != NULL) {
    ofp = fopen(out_path, "w");
  }
  
	// seek = start_secs * ft->signal.rate * ft->signal.channels + .5;
	// seek -= seek % ft->signal.channels;
	sox_seek(ft, 0, SOX_SEEK_SET);

	// block_size = 0.05 * ft->signal.rate * ft->signal.channels + .5;
	// block_size -= block_size % ft->signal.channels;
  block_size = ANALYSIS_WINDOW * ft->signal.channels;
	buf = malloc(sizeof(sox_sample_t) * block_size);

  // 現在は1チャンネル目のみ
	for(blocks = 0; (readed = sox_read(ft, buf, block_size)) && readed > 0; ++blocks) {
		// printf("\n--- blocks: %ld, readed: %ld ---\n", blocks, readed);
    tmp = 0;
    end_block = false;

		for(i = 0; i < block_size;) {
			SOX_SAMPLE_LOCALS;
			// int sample_r = SOX_SAMPLE_TO_UNSIGNED_16BIT(buf[i++], ft->clips);
			// int sample_l = SOX_SAMPLE_TO_UNSIGNED_16BIT(buf[i++], ft->clips);
      if (i >= readed) {
        end_block = true;
        break;
      }

      vals_r[tmp] = SOX_SAMPLE_TO_UNSIGNED_16BIT(buf[i++], ft->clips);
      if (ft->signal.channels == 2) {
        i++;
      }
      tmp++;
		}
    
    if (end_block == false) {
      analyze_signal(vals_r, mfd_array);
      len = sizeof(mfd_array) / sizeof(mfd_array[0]) - 1;

      if (ofp != NULL) {
        for (i = 0; i < len; i++) {
          fprintf(ofp, "%f,", mfd_array[i]);
        }
        fprintf(ofp, "%f\n", mfd_array[len]);
      } else {
        for (i = 0; i < len; i++) {
          printf("%f,", mfd_array[i]);
        }
        printf("%f\n", mfd_array[len]);
      }
    }
	}

  // make histgrams
  if (ofp != NULL) {
    fclose(ofp);
  }
  
  printf("\n");

	free(buf);
	sox_close(ft);
	sox_quit();
  
  exit(0);
}

void analyze_signal(unsigned short vals[], double mfd[])
{
  int16_t ww, hh, canv_size, canv_offset, mfd_idx;
	//uint16_t *canv, *circle;
  int32_t *canv, *circle;
  //int32_t num_vals = sizeof(vals) / sizeof(vals[0]);
  
  int32_t phase = 0, c_radius, last_c_radius;
	int32_t p, i, xx, yy;
	int32_t lt_x, ci_x, canv_idx, ci_idx;
  
	int64_t total_area_sum, last_total_area_sum;

	double total_dist, c_dist;

  mfd_idx = 0;
  last_total_area_sum = 0;

  /* for (c_radius = RADIUS * RADIUS_MULT_MIN; c_radius <= RADIUS * RADIUS_MULT_MAX; c_radius += RADIUS) { */
  for (p = RADIUS_MULT_MIN; p <= RADIUS_MULT_MAX; p += 1) {   
    /*
     * create circle unit
     */

    c_radius = round(pow(1.4, (double)p));

    ww = (int16_t)c_radius * 2 + 1;
    hh = (int16_t)c_radius * 2 + 1;
    canv_size = ww * 2;
    circle = calloc(ww, sizeof(int32_t));

    if(circle == NULL)
      puts("failed to calloc of circle");

    ci_idx = 0;
    /*for(xx = 0; xx < ww; xx++) {
      tmp = 0;
      for(yy = 0; yy < hh; yy++) {			
        c_dist = sqrt(pow((c_radius - xx), 2) + pow((c_radius - yy), 2));
        if(tmp == 0 && min(1.0, c_radius - c_dist) >= 0.5) {
          circle[ci_idx++] = yy;
          tmp = 1;
        } else if (tmp == 1 && min(1.0, c_radius - c_dist) < 0.5) {
          circle[ci_idx++] = yy - 1;
          break;
        } else if (tmp == 1 && yy == hh - 1) {
          circle[ci_idx++] = yy;
        }
      }
    }
    */
    for (xx = -c_radius; xx <= 0; xx++) {
      for (yy = c_radius; yy >= 0; yy--) {
        c_dist = sqrt(pow(xx, 2) + pow(yy, 2));
        if (c_radius >= c_dist) {
          circle[ci_idx++] = yy;
          break;
        }
      }
    }
    
    for (xx = 1; xx <= c_radius; xx++) {
      circle[ci_idx] = circle[ww - ci_idx - 1];
      ci_idx++;
    }

    /*
     * create calculation canvas
     */
    canv = calloc(canv_size, sizeof(int32_t));

    if(canv == NULL)
      puts("failed to calloc of canv");

    canv_idx = 0;
    for (xx = 0; xx < ww; xx++) {
      canv[canv_idx++] = INT32_MAX;
      canv[canv_idx++] = INT32_MIN;
    }

    phase = 1;
    canv_offset = 0;
    total_dist = 0.0;
    total_area_sum = 0;
    
    //printf("samples: ");
    
    lt_x = - c_radius;
    for(i = 0; i < ANALYSIS_WINDOW; i++) {
        /*
         * phase 1
         * circle move to the middle point of canvas
         */
        
        if(phase == 1) {
          
          if(lt_x >= 0) {
            phase = 2;
          }

          ci_idx = -lt_x * 2;
          canv_idx = 0;
          
          for (ci_x = -lt_x; ci_x < ww; ci_x++) {
            if (canv[canv_idx] > -circle[ci_x] + vals[i]) {
              canv[canv_idx] = -circle[ci_x] + vals[i];
            }
            canv_idx++;
            
            if (canv[canv_idx] < circle[ci_x] + vals[i]) {
              canv[canv_idx] = circle[ci_x] + vals[i];
            }
            canv_idx++;
          }
          
          lt_x++;
        } else if(phase == 2) { /* << phase 1 */
          
          /*
           * phase 2
           * draw sequential process
           */

          total_area_sum += (canv[canv_offset + 1] - canv[canv_offset]);
          canv[canv_offset++] = INT32_MAX;
          canv[canv_offset++] = INT32_MIN;

          if (canv_offset >= canv_size) {
            canv_offset = 0;
          }

          canv_idx = canv_offset;
          for (ci_x = 0; ci_x < ww; ci_x++) {
            if (canv[canv_idx] > -circle[ci_x] + vals[i]) {
              canv[canv_idx] = -circle[ci_x] + vals[i];
            }
            canv_idx++;
            
            if (canv[canv_idx] < circle[ci_x] + vals[i]) {
              canv[canv_idx] = circle[ci_x] + vals[i];
            }
            canv_idx++;
            
            if (canv_idx >= canv_size) {
              canv_idx = 0;
            }
          }
        } /* << phase 2 */
    } /* << samples loop */
    
    phase = 3;
    
    /*
     * phase 3
     * finalize
     */
    
    canv_idx = canv_offset;
    for (i = 0; i < c_radius; i++) {
      total_area_sum += (canv[canv_idx + 1] - canv[canv_idx]);
      canv_idx += 2;
      
      if (canv_idx >= canv_size) {
        canv_idx = 0;
      }
    }

	  free(canv);
	  free(circle);

	  //printf("%d\t%lld\n", c_radius, total_area_sum);
    //printf("%lld, ", total_area_sum);
    
    if (last_total_area_sum > 0) {
      mfd[mfd_idx++] = 2.0 - log((double)total_area_sum / last_total_area_sum) / log((double)c_radius / last_c_radius);
    }
    last_c_radius = c_radius;
    last_total_area_sum = total_area_sum;
  }

	return;
}


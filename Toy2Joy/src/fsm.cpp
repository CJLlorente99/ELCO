/*
 * fsm.c
 *
 *  Created on: 1 de mar. de 2016
 *      Author: Administrador
 */

#include <stdlib.h>
#include "fsm.h"

fsm_t*
fsm_new (int state, fsm_trans_t* tt, void* user_data)
{
  fsm_t* fsm = (fsm_t*) malloc (sizeof (fsm_t));
  fsm_init (fsm, state, tt, user_data);
  return fsm;
}

void
fsm_init (fsm_t* fsm, int state, fsm_trans_t* tt, void* user_data)
{
  fsm->current_state = state;
  fsm->tt = tt;
  fsm->user_data = user_data;
}

void
fsm_destroy (fsm_t* fsm)
{
  free(fsm);
}

void
fsm_fire (fsm_t* fsm)
{
  fsm_trans_t* t;
  for (t = fsm->tt; t->orig_state >= 0; ++t) {
    if ((fsm->current_state == t->orig_state) && t->in(fsm)) {
      fsm->current_state = t->dest_state;
      if (t->out)
        t->out(fsm);
      break;
    }
  }
}


/* ctwstep - calculates stepsize which is a measure for the probability of the bit */

/* STEPHALF defines which stepsize is equivalent to a probability of 0.5 */
#define STEPHALF (ARENTRIES)

/* calculates *instep and *symbsmall from ctwprob. *instep is the stepsize that corresponds to
   the probability of the most probable symbol. In *symbsmall, the symbol with the smallest
   probability is returned */
void CTWsteps(struct CTWProb ctwprob, int *instep, boolean *symbsmall);

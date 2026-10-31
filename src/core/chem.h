/* chem.h - moles, solutions and gases (V2 feature F11).
 *
 * Spec: the V2 master prompt, F11.  Three families of calculation, each with
 * its own triangle, plus the two percentage measures a paper asks for:
 *
 *      n = m / Mr              moles from a mass
 *      c = n / V               concentration, V in dm^3
 *      p V = n R T             ideal gas, p in kPa, V in dm^3, T in K
 *      c1 V1 = c2 V2           dilution
 *      yield = actual / theoretical, atom economy = Mr(wanted) / Mr(all)
 *
 * Every function returns false rather than a division by nothing: Mr, volume
 * and temperature are the three denominators a student can type as zero, and a
 * silent infinity is the wrong answer to any of them.
 *
 * No gint, no libm.
 */

#ifndef BENCH_CHEM_H
#define BENCH_CHEM_H

#include <stdbool.h>

/* R for p in kPa, V in dm^3, T in K. */
#define CHEM_R 8.314462618

/* m = n Mr */
bool chem_mass(double n, double mr, double *out);
/* n = m / Mr */
bool chem_moles(double mass, double mr, double *out);
/* Mr = m / n */
bool chem_mr(double mass, double n, double *out);

/* c = n / V, V in dm^3 */
bool chem_conc(double n, double v_dm3, double *out);
/* n = c V */
bool chem_n_from_conc(double c, double v_dm3, double *out);
/* V = n / c */
bool chem_volume_from_conc(double n, double c, double *out);

/* p = n R T / V, kPa out */
bool chem_gas_pressure(double n, double t_k, double v_dm3, double *out);
/* V = n R T / p */
bool chem_gas_volume(double n, double t_k, double p_kpa, double *out);
/* T = p V / (n R) */
bool chem_gas_temp(double n, double p_kpa, double v_dm3, double *out);
/* n = p V / (R T) */
bool chem_gas_moles(double p_kpa, double v_dm3, double t_k, double *out);

/* c2 = c1 V1 / V2 */
bool chem_dilute(double c1, double v1, double v2, double *out);

/* percentages, given the two masses (or MRs) */
bool chem_yield(double actual, double theoretical, double *out);
bool chem_atom_economy(double mr_wanted, double mr_all, double *out);

/* cm^3 <-> dm^3, because a paper gives one and the formula wants the other. */
double chem_cm3_to_dm3(double cm3);
double chem_dm3_to_cm3(double dm3);

#endif /* BENCH_CHEM_H */

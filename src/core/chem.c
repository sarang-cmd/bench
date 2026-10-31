/* chem.c - moles, solutions and gases.  See chem.h. */

#include "chem.h"

#include <stddef.h>

static bool good(double x)
{
    return x > 0 && x < 1e300;
}

bool chem_mass(double n, double mr, double *out)
{
    if (!good(n) || !good(mr)) return false;
    *out = n * mr;
    return true;
}

bool chem_moles(double mass, double mr, double *out)
{
    if (!good(mass) || !good(mr)) return false;
    *out = mass / mr;
    return true;
}

bool chem_mr(double mass, double n, double *out)
{
    if (!good(mass) || !good(n)) return false;
    *out = mass / n;
    return true;
}

bool chem_conc(double n, double v_dm3, double *out)
{
    if (!good(n) || !good(v_dm3)) return false;
    *out = n / v_dm3;
    return true;
}

bool chem_n_from_conc(double c, double v_dm3, double *out)
{
    if (!good(c) || !good(v_dm3)) return false;
    *out = c * v_dm3;
    return true;
}

bool chem_volume_from_conc(double n, double c, double *out)
{
    if (!good(n) || !good(c)) return false;
    *out = n / c;
    return true;
}

bool chem_gas_pressure(double n, double t_k, double v_dm3, double *out)
{
    if (!good(n) || !good(t_k) || !good(v_dm3)) return false;
    *out = n * CHEM_R * t_k / v_dm3;
    return true;
}

bool chem_gas_volume(double n, double t_k, double p_kpa, double *out)
{
    if (!good(n) || !good(t_k) || !good(p_kpa)) return false;
    *out = n * CHEM_R * t_k / p_kpa;
    return true;
}

bool chem_gas_temp(double n, double p_kpa, double v_dm3, double *out)
{
    if (!good(n) || !good(p_kpa) || !good(v_dm3)) return false;
    *out = p_kpa * v_dm3 / (n * CHEM_R);
    return true;
}

bool chem_gas_moles(double p_kpa, double v_dm3, double t_k, double *out)
{
    if (!good(p_kpa) || !good(v_dm3) || !good(t_k)) return false;
    *out = p_kpa * v_dm3 / (CHEM_R * t_k);
    return true;
}

bool chem_dilute(double c1, double v1, double v2, double *out)
{
    if (!good(c1) || !good(v1) || !good(v2)) return false;
    *out = c1 * v1 / v2;
    return true;
}

bool chem_yield(double actual, double theoretical, double *out)
{
    if (actual < 0 || !good(theoretical)) return false;
    *out = actual / theoretical * 100.0;
    return true;
}

bool chem_atom_economy(double mr_wanted, double mr_all, double *out)
{
    if (!good(mr_wanted) || !good(mr_all)) return false;
    *out = mr_wanted / mr_all * 100.0;
    return true;
}

double chem_cm3_to_dm3(double cm3)
{
    return cm3 / 1000.0;
}

double chem_dm3_to_cm3(double dm3)
{
    return dm3 * 1000.0;
}

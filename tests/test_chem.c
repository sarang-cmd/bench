/* test_chem.c - moles, solutions and gases (V2 feature F11).
 *
 * Vectors are the ones a data booklet gives: NaOH is 40 g/mol, one mole of gas
 * in 24 dm^3 at 298 K is about 103 kPa, and diluting 20 cm^3 of 0.5 mol/dm^3
 * into 250 cm^3 gives 0.04 mol/dm^3.
 */

#include "test_util.h"

#include "chem.h"

static void test_moles(void)
{
    double x;

    T_TRUE(chem_moles(4.0, 40.0, &x));
    T_NEAR(x, 0.1, 1e-12);

    T_TRUE(chem_mass(0.1, 40.0, &x));
    T_NEAR(x, 4.0, 1e-12);

    T_TRUE(chem_mr(4.0, 0.1, &x));
    T_NEAR(x, 40.0, 1e-12);

    /* 2.5 g of CaCO3, Mr 100.09. */
    T_TRUE(chem_moles(2.5, 100.09, &x));
    T_NEAR(x, 0.024977, 1e-6);

    T_FALSE(chem_moles(4.0, 0.0, &x));
    T_FALSE(chem_mass(0.0, 40.0, &x));
    T_FALSE(chem_mr(4.0, 0.0, &x));
}

static void test_solutions(void)
{
    double x;

    T_TRUE(chem_conc(0.1, 0.25, &x));
    T_NEAR(x, 0.4, 1e-12);

    T_TRUE(chem_n_from_conc(0.4, 0.25, &x));
    T_NEAR(x, 0.1, 1e-12);

    T_TRUE(chem_volume_from_conc(0.1, 0.4, &x));
    T_NEAR(x, 0.25, 1e-12);

    /* 25 cm^3 is 0.025 dm^3, and 0.05 mol in it is 2 mol/dm^3. */
    T_NEAR(chem_cm3_to_dm3(25), 0.025, 1e-12);
    T_NEAR(chem_dm3_to_cm3(0.25), 250, 1e-12);
    T_TRUE(chem_conc(0.05, chem_cm3_to_dm3(25), &x));
    T_NEAR(x, 2.0, 1e-12);

    T_FALSE(chem_conc(0.1, 0.0, &x));
    T_FALSE(chem_volume_from_conc(0.1, 0.0, &x));
}

static void test_gas(void)
{
    double x;

    T_TRUE(chem_gas_pressure(1.0, 298.0, 24.0, &x));
    T_NEAR(x, 8.314462618 * 298.0 / 24.0, 1e-9);
    T_NEAR(x, 103.238, 5e-3);

    T_TRUE(chem_gas_volume(1.0, 298.0, 100.0, &x));
    T_NEAR(x, 24.777, 5e-3);

    T_TRUE(chem_gas_temp(1.0, 100.0, 24.0, &x));
    T_NEAR(x, 2400.0 / 8.314462618, 1e-9);

    T_TRUE(chem_gas_moles(100.0, 24.0, 298.0, &x));
    T_NEAR(x, 2400.0 / (8.314462618 * 298.0), 1e-12);
    T_NEAR(x, 0.96863, 1e-5);

    /* 273 K and 101 kPa: 22.4 dm^3 is about one mole. */
    T_TRUE(chem_gas_moles(101.0, 22.4, 273.0, &x));
    T_NEAR(x, 0.9975, 1e-3);

    T_FALSE(chem_gas_pressure(1.0, 298.0, 0.0, &x));
    T_FALSE(chem_gas_moles(100.0, 24.0, 0.0, &x));
}

static void test_percentages(void)
{
    double x;

    T_TRUE(chem_dilute(0.5, 20.0, 250.0, &x));
    T_NEAR(x, 0.04, 1e-12);

    T_TRUE(chem_yield(3.5, 5.0, &x));
    T_NEAR(x, 70.0, 1e-12);

    T_TRUE(chem_atom_economy(44.0, 62.0, &x));
    T_NEAR(x, 70.96774193548387, 1e-9);

    T_TRUE(chem_yield(0.0, 5.0, &x));
    T_NEAR(x, 0.0, 1e-12);

    T_FALSE(chem_yield(3.5, 0.0, &x));
    T_FALSE(chem_dilute(0.5, 20.0, 0.0, &x));
}

static void run_tests(void)
{
    T_SECTION("moles and Mr");
    test_moles();
    T_SECTION("solutions");
    test_solutions();
    T_SECTION("the gas equation");
    test_gas();
    T_SECTION("dilution, yield and atom economy");
    test_percentages();
}

T_MAIN("test_chem")

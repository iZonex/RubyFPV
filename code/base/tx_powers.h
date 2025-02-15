#pragma once

#include "hardware_radio.h"
#include "models.h"

// Max deviation to show a value in ui as a standard tx value, in percentages
#define MAX_TX_POWER_UI_DEVIATION_FROM_STANDARD 12

const int* tx_powers_get_raw_radio_power_values(int* piOutputCount);
const int* tx_powers_get_ui_levels_mw(int* piOutputCount);
const int* tx_powers_get_raw_measurement_intervals(int* piOutputCount);
const int* tx_powers_get_mw_measured_values_for_card(int iDriverType, int iCardModel, int* piOutputCount);

int tx_powers_get_max_usable_power_mw_for_card(int iDriverType, int iCardModel);
int tx_powers_get_max_usable_power_raw_for_card(int iDriverType, int iCardModel);
int tx_powers_convert_raw_to_mw(int iDriverType, int iCardModel, int iRawPower);
int tx_powers_convert_mw_to_raw(int iDriverType, int iCardModel, int imWPower);

int tx_powers_get_max_usable_power_mw_for_model(Model* pModel);
void tx_power_get_current_mw_powers_for_model(Model* pModel, int* piOutputArray);

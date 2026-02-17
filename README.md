# Usage
This was made for IYPT 2026 problem no. 17.
## Vstupy
Vytisknout teploty: `t`

Zapnout zapalovac na DEFAULT_LIGHTER_MILIS: `f`

Zapnout zapalovac na `<l>` (n+1)krat s prodlevama d: `d <l> <d_1> <d_2> ... <d_n>`

Termostat ON/OFF: `g`

Nastavit termostat: `s <t_C>`

Topeni ON/OFF: `h`

## Vystupy
Teploty termistoru v °C: `T <t_1> <t_2> ... <t_n>`

Zapalovac zapnut: `S <timestampMs>`

Zapalovac vypnut: `E <elapsedTimeUs> <timestampMs>`

Zmena na senzoru: `F <0|1> <timestampMs>`

Ostatni: `O <text>`
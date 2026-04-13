This was made for IYPT 2026 problem no. 17.

> 17. Travelling flame: A flame can propagate continuously around a ring-shaped trough containing a thin layer of flammable liquid. Investigate how the characteristics of this travelling flame depend on relevant parameters.

> 17. Cestující plamen: Plamen může nepřetržitě obíhat žlábkem stočeným do kruhu, který obsahuje tenkou vrstvu hořlavé kapaliny. Prozkoumejte, jak charakteristiky tohoto cestujícího plamene závisejí na relevantních parametrech. 

# Použití
Komunikace mezi PC a arduinem probíhá sériově přes USB na baud rate 9600.
Příkaz arduinu je buď znak (např. `f`, `h`, `g`), nebo znak následovaný jedním nebo více čísly (např. `d 200 1000 1000`, `s 30`). Pokud se příkaz skládá z více znaků, musí být ukončen novým řádkem. Výstup z arduina začína znakem označujícím typ zprávy a pak hodnoty oddělené mezerou (např. `T 21.5 22.3 21.6 16.7 H`).

Komunikace s arduinem lze také provést přes python program `client/main.py`, který vyžaduje knihovnu `pyserial` (instalace `pip install pyserial`), spuštení `python client/main.py <PORT>`. Program automaticky ukládá naměrená data do `client/out`.

**Dokumentace zapojení je [zde](docs/setup.md)**

## Vstupy
Vytisknout teploty: `t`

Zapnout zapalovac na DEFAULT_LIGHTER_MILIS: `f`

Zapnout zapalovac na `<l>`ms (n+1)krat s prodlevama `<d_i>`ms: `d <l> <d_1> <d_2> ... <d_n>`

Termostat ON/OFF: `g`

Nastavit termostat: `s <t_C>`

Topeni ON/OFF: `h`

Test změny teploty: `1` začne rapidně vypisovat teplotu a po třech vteřínách pošle plamen. Ukončí měření když dostane jakýkoliv znak na vstupu

## Výstupy
Teploty termistoru v °C: `T <t_1> <t_2> ... <t_n>` + `H` pokud diody zrovna topí

Teploty termistoru v °C: `I <t_1> <t_2> ... <t_n>`

Zapalovač zapnut: `S <timestampMs>`

Zapalovač vypnut: `E <elapsedTimeUs> <timestampMs>`

Změna na senzoru: `F <0|1> <timestampMs>`

Debug zprávy: `O <text>`
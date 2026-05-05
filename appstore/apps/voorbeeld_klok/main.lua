-- BKOS Voorbeeld App: Klok
-- Laat zien hoe je bkos.* API gebruikt voor scherm, IO en data.
-- Ontwerp voor 800x480 scherm.

-- Kleuren ophalen uit huidige palette
local C_BG      = bkos.kleur.bg
local C_TEXT    = bkos.kleur.tekst
local C_DIM     = bkos.kleur.tekst_dim
local C_CYAAN   = bkos.kleur.cyaan
local C_GROEN   = bkos.kleur.groen

-- Lamp-naam die via IO bediend kan worden (aanpassen aan jouw installatie)
local LAMP_NAAM = "salon"

-- Teken het scherm
function bkos.teken()
    -- Achtergrond
    bkos.vul(0, 0, bkos.W, bkos.H, C_BG)

    -- Tijd uit data-opslag (door NTP bijgehouden, leeftijd <5s = vers)
    local tijdstring = bkos.data.lees("sys.tijd") or "--:--"
    local datumstring = bkos.data.lees("sys.datum") or ""

    -- Grote klok in het midden
    bkos.tekst(bkos.W/2 - 120, 100, tijdstring, 8, C_CYAAN)

    -- Datum daaronder
    bkos.tekst(bkos.W/2 - 80, 220, datumstring, 2, C_DIM)

    -- Temperatuur uit meteo data-opslag
    local temp = bkos.data.lees_f("meteo.temp")
    local temp_str
    if temp ~= 0 then
        temp_str = string.format("%.1f C", temp)
    else
        temp_str = "- C"
    end
    bkos.tekst(bkos.W/2 - 50, 270, temp_str, 3, C_TEXT)

    -- IO lamp-knop onderin
    local lamp_staat = bkos.io.lees_naam(LAMP_NAAM)
    local knop_kleur = lamp_staat and C_GROEN or bkos.rgb(60, 70, 90)
    bkos.vul(bkos.W/2 - 80, 340, 160, 60, knop_kleur)
    bkos.tekst(bkos.W/2 - 36, 362, lamp_staat and "AAN" or "UIT", 2, C_TEXT)
    bkos.tekst(bkos.W/2 - 22, 384, LAMP_NAAM, 1, C_DIM)
end

-- Aanraking verwerken
function bkos.aanraking(x, y)
    -- Lamp-knop aangeraakt?
    if x >= bkos.W/2 - 80 and x <= bkos.W/2 + 80 and
       y >= 340 and y <= 400 then
        bkos.io.wissel_naam(LAMP_NAAM)
        bkos.teken()  -- direct hertekenen
    end
end

-- Periodieke update (elke loop-iteratie zonder aanraking)
local laatste_teken = 0
function bkos.update()
    -- Herteken elke 10 seconden voor verse tijd
    if bkos.sys.millis() - laatste_teken > 10000 then
        laatste_teken = bkos.sys.millis()
        bkos.teken()
    end
end

-- ─────────────────────────────────────────────────────────────────────────────
-- BKOS App: Boter Kaas & Eieren
-- Twee spelers, beurtelings aanraken op het touchscherm.
-- Demonstreert: bkos.teken, bkos.aanraking, bkos.cirkel, bkos.lijn, bkos.vul
-- ─────────────────────────────────────────────────────────────────────────────

-- Constanten
local LEEG     = 0
local SPELER_X = 1
local SPELER_O = 2

-- Spelstatus
local bord            = {}
local huidige_speler  = SPELER_X
local winnaar         = LEEG
local spel_klaar      = false
local gelijkspel      = false

-- Layout (ontworpen voor 800×480; header/footer worden door BKOS-NUI beheerd)
local CEL     = 120        -- pixels per cel
local GRID_X  = (bkos.W - 3 * CEL) / 2   -- horizontaal gecentreerd = 220
local GRID_Y  = 48         -- ruimte voor status-/beurt-tekst bovenaan
local RAND    = 16         -- marge binnen elke cel voor symbool

-- Kleuren
local C_RASTER  = bkos.rgb(60,  85, 110)
local C_X       = bkos.kleur.rood
local C_O       = bkos.kleur.cyaan
local C_WIN     = bkos.kleur.groen
local C_KNOP    = bkos.rgb(40,  60,  80)
local C_KNOP_T  = bkos.kleur.tekst

-- Hulpfunctie: cel-index (1-gebaseerd) voor rij 0-2, kolom 0-2
local function idx(r, k)  return r * 3 + k + 1  end

-- ─── Symbolen tekenen ────────────────────────────────────────────────────────
local function teken_x(cx, cy)
    local h = CEL / 2 - RAND
    -- Twee diagonalen, elk 3px breed
    for d = -1, 1 do
        bkos.lijn(cx - h + d, cy - h, cx + h + d, cy + h, C_X)
        bkos.lijn(cx + h + d, cy - h, cx - h + d, cy + h, C_X)
    end
end

local function teken_o(cx, cy)
    local r = CEL / 2 - RAND
    -- Dubbele cirkel voor dikte
    bkos.cirkel(cx, cy, r,     C_O, false)
    bkos.cirkel(cx, cy, r - 1, C_O, false)
    bkos.cirkel(cx, cy, r - 2, C_O, false)
end

-- ─── Wincontrole ─────────────────────────────────────────────────────────────
local WIN_LIJNEN = {
    {1,2,3}, {4,5,6}, {7,8,9},   -- rijen
    {1,4,7}, {2,5,8}, {3,6,9},   -- kolommen
    {1,5,9}, {3,5,7}             -- diagonalen
}

local function controleer_win()
    for _, lijn in ipairs(WIN_LIJNEN) do
        local a, b, c = bord[lijn[1]], bord[lijn[2]], bord[lijn[3]]
        if a ~= LEEG and a == b and b == c then return a end
    end
    return LEEG
end

local function is_vol()
    for i = 1, 9 do
        if bord[i] == LEEG then return false end
    end
    return true
end

-- ─── Bord initialiseren ───────────────────────────────────────────────────────
local function nieuw_spel()
    bord = {}
    for i = 1, 9 do bord[i] = LEEG end
    huidige_speler = SPELER_X
    winnaar        = LEEG
    spel_klaar     = false
    gelijkspel     = false
end

nieuw_spel()

-- ─── Scherm tekenen ──────────────────────────────────────────────────────────
function bkos.teken()
    -- Achtergrond
    bkos.vul(0, 0, bkos.W, bkos.H, bkos.kleur.bg)

    -- Status / beurt aanduiding bovenaan
    local status, s_kleur
    if spel_klaar then
        if winnaar ~= LEEG then
            status  = (winnaar == SPELER_X) and "Speler X wint!" or "Speler O wint!"
            s_kleur = C_WIN
        else
            status  = "Gelijkspel — OPNIEUW?"
            s_kleur = bkos.kleur.amber
        end
    elseif huidige_speler == SPELER_X then
        status  = "Speler X is aan zet"
        s_kleur = C_X
    else
        status  = "Speler O is aan zet"
        s_kleur = C_O
    end
    -- Achtergrond statusbalk
    bkos.vul(0, 0, bkos.W, GRID_Y - 4, bkos.rgb(18, 26, 36))
    -- Centreer de tekst (size 2 = 12px per karakter)
    local tx = math.floor((bkos.W - #status * 12) / 2)
    bkos.tekst(tx, 18, status, 2, s_kleur)

    -- Rasterlijnen (4px breed)
    for i = 1, 2 do
        bkos.vul(GRID_X + i * CEL - 2, GRID_Y,     4, 3 * CEL, C_RASTER)  -- verticaal
        bkos.vul(GRID_X,               GRID_Y + i * CEL - 2, 3 * CEL, 4, C_RASTER)  -- horizontaal
    end

    -- Cel-inhoud
    for r = 0, 2 do
        for k = 0, 2 do
            local v  = bord[idx(r, k)]
            local cx = GRID_X + k * CEL + math.floor(CEL / 2)
            local cy = GRID_Y + r * CEL + math.floor(CEL / 2)
            if v == SPELER_X then teken_x(cx, cy)
            elseif v == SPELER_O then teken_o(cx, cy)
            end
        end
    end

    -- OPNIEUW knop onderaan het raster
    local kx = math.floor(bkos.W / 2) - 90
    local ky = GRID_Y + 3 * CEL + 18
    bkos.vul(kx, ky, 180, 48, C_KNOP)
    bkos.tekst(kx + 16, ky + 16, "OPNIEUW", 2, C_KNOP_T)
end

-- ─── Aanraking verwerken ─────────────────────────────────────────────────────
function bkos.aanraking(x, y)
    -- OPNIEUW knop
    local kx = math.floor(bkos.W / 2) - 90
    local ky = GRID_Y + 3 * CEL + 18
    if x >= kx and x <= kx + 180 and y >= ky and y <= ky + 48 then
        nieuw_spel()
        bkos.teken()
        return
    end

    if spel_klaar then return end

    -- Cel aangeraakt?
    if x >= GRID_X and x < GRID_X + 3 * CEL and
       y >= GRID_Y and y < GRID_Y + 3 * CEL then
        local k = math.floor((x - GRID_X) / CEL)
        local r = math.floor((y - GRID_Y) / CEL)
        local i = idx(r, k)
        if bord[i] == LEEG then
            bord[i] = huidige_speler
            winnaar  = controleer_win()
            if winnaar ~= LEEG then
                spel_klaar = true
            elseif is_vol() then
                gelijkspel = true
                spel_klaar = true
            else
                huidige_speler = (huidige_speler == SPELER_X) and SPELER_O or SPELER_X
            end
            bkos.teken()
        end
    end
end

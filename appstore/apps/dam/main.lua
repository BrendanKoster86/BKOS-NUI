-- ============================================================
--  DAMSPEL  v1.0  –  BKOS Lua App API v2
--  800 × 480 designresolutie
--  Internationaal damspel: 10×10 bord, verplicht slaan,
--  meerslaan, damstuk (vrij bewegen over diagonalen)
--
--  Scherm-layout:
--    Bord  : x=10, y=5,  10×10 vakjes van 46×46 = 460×460
--    Panel : x=482, y=5, breedte ~308
-- ============================================================

-- ── Kleurpalet ──────────────────────────────────────────────
local C = {
  bg        = bkos.color565( 18,  24,  38),
  panel     = bkos.color565( 28,  36,  54),
  border    = bkos.color565( 60,  80, 120),
  lichtVak  = bkos.color565(240, 217, 181),  -- speelt niet mee (licht)
  donkerVak = bkos.color565( 80,  52,  32),  -- speelveld (donker)
  selectie  = bkos.color565( 80, 200,  60),
  mogelijkH = bkos.color565( 60, 160, 230),
  slagpad   = bkos.color565(230, 140,  30),
  tekst     = bkos.color565(220, 220, 235),
  tekstDim  = bkos.color565(120, 130, 160),
  knopActi  = bkos.color565( 60, 140, 220),
  knopInac  = bkos.color565( 40,  55,  85),
  knopTek   = bkos.color565(240, 245, 255),
  highlight = bkos.color565(255, 210,  50),
  lastMove  = bkos.color565(160, 140,  30),
  witStuk   = bkos.color565(235, 235, 220),
  zwartStuk = bkos.color565( 30,  20,  10),
  witRand   = bkos.color565(180, 170, 150),
  zwartRand = bkos.color565( 80,  60,  40),
  damKroon  = bkos.color565(255, 200,   0),
  check     = bkos.color565(220,  50,  40),
}

-- ── Bord-layout ──────────────────────────────────────────────
local BX = 10   -- bord linker x
local BY =  5   -- bord boven y
local SQ = 46   -- vakgrootte (10×46 = 460)

-- ── Stuk types ────────────────────────────────────────────────
-- bord[r][k]: 0=leeg, 1=wit schijf, 2=zwart schijf, 3=witte dam, 4=zwarte dam
local LEEG=0; local WIT=1; local ZWART=2; local WIT_DAM=3; local ZWART_DAM=4

local function isWit(s)   return s==WIT   or s==WIT_DAM   end
local function isZwart(s) return s==ZWART or s==ZWART_DAM end
local function isDam(s)   return s==WIT_DAM or s==ZWART_DAM end
local function kleurVan(s)
  if isWit(s)   then return 1
  elseif isZwart(s) then return -1
  else return 0 end
end

-- ── Speelstatus ───────────────────────────────────────────────
local MENU   = 0
local KLEUR  = 1
local NIVEAU = 2
local SPEL   = 3

-- ── Staat ────────────────────────────────────────────────────
local staat = MENU

local bord = {}
local aanBeurt = 1        -- 1=wit, -1=zwart
local geselecteerd = nil  -- {r,k}
local mogelijkeZetten = {} -- lijst {r,k} voor geselecteerde schijf
local slagVerplicht = false
local berichtTekst = ""
local laatstzet = nil     -- {r0,k0,r1,k1}
local matOfPat = ""       -- "mat"|"pat"|""

-- Meerslaan: als je aan het slaan bent, moet je op dezelfde schijf blijven
local meerslaanActief = false
local meerslaanStuk   = nil   -- {r,k}

-- ── Spelmodus ────────────────────────────────────────────────
local tegenComputer = false
local spelersKleur  = 1
local niveauComp    = 2

-- ── Geslagen tellers ─────────────────────────────────────────
local geslagenWit   = 0
local geslagenZwart = 0

-- =============================================================
--  HULPFUNCTIES
-- =============================================================

local function inBord(r,k)
  return r>=1 and r<=10 and k>=1 and k<=10
end

local function isDonkerVak(r,k)
  -- In dam wordt gespeeld op donkere vakken: (r+k) oneven
  return (r+k) % 2 == 1
end

local function kopieerBord(b)
  local n = {}
  for r=1,10 do
    n[r] = {}
    for k=1,10 do n[r][k] = b[r][k] end
  end
  return n
end

-- =============================================================
--  STARTPOSITIE
-- =============================================================

local function initialiseerBord()
  bord = {}
  for r=1,10 do
    bord[r] = {}
    for k=1,10 do bord[r][k] = LEEG end
  end

  -- Zwart: rijen 1-4 op donkere vakken
  for r=1,4 do
    for k=1,10 do
      if isDonkerVak(r,k) then bord[r][k] = ZWART end
    end
  end
  -- Wit: rijen 7-10 op donkere vakken
  for r=7,10 do
    for k=1,10 do
      if isDonkerVak(r,k) then bord[r][k] = WIT end
    end
  end

  aanBeurt = 1
  geselecteerd = nil
  mogelijkeZetten = {}
  slagVerplicht = false
  berichtTekst = ""
  laatstzet = nil
  matOfPat = ""
  meerslaanActief = false
  meerslaanStuk = nil
  geslagenWit = 0
  geslagenZwart = 0
end

-- =============================================================
--  ZETGENERATIE
-- =============================================================

-- Gewone zetten voor schijf op (r,k) – niet slagzetten
local function gewoneZetten(b, r, k)
  local stuk = b[r][k]
  local zetten = {}
  if stuk == LEEG then return zetten end

  if isDam(stuk) then
    -- Dam: beweegt vrij over diagonalen (tot blokkade)
    local dirs = {{1,1},{1,-1},{-1,1},{-1,-1}}
    for _, d in ipairs(dirs) do
      local cr, ck = r+d[1], k+d[2]
      while inBord(cr,ck) and b[cr][ck]==LEEG do
        zetten[#zetten+1] = {cr, ck}
        cr = cr+d[1]; ck = ck+d[2]
      end
    end
  else
    -- Schijf: beweegt alleen vooruit (wit=omhoog rij-1, zwart=omlaag rij+1)
    local richting = isWit(stuk) and -1 or 1
    for _, dk in ipairs({-1, 1}) do
      local nr, nk = r+richting, k+dk
      if inBord(nr,nk) and b[nr][nk]==LEEG then
        zetten[#zetten+1] = {nr, nk}
      end
    end
  end
  return zetten
end

-- Slagzetten voor schijf op (r,k), geeft {nr,nk,sr,sk} terug
-- sr,sk = geslagen schijf positie
local function slagzettenVanaf(b, r, k, uitgesloten)
  -- uitgesloten: set van "r,k" strings die al geslagen zijn (meerslaan)
  local stuk = b[r][k]
  local zetten = {}
  if stuk == LEEG then return zetten end
  local kleur = kleurVan(stuk)
  uitgesloten = uitgesloten or {}

  if isDam(stuk) then
    local dirs = {{1,1},{1,-1},{-1,1},{-1,-1}}
    for _, d in ipairs(dirs) do
      -- Zoek langs diagonaal naar een slaanbare schijf
      local cr, ck = r+d[1], k+d[2]
      local gevonden = false
      while inBord(cr,ck) do
        local t = b[cr][ck]
        if t ~= LEEG then
          if kleurVan(t) == -kleur and not uitgesloten[cr..","..ck] then
            -- Sla over: land op alle lege vakken erna
            local lr, lk = cr+d[1], ck+d[2]
            while inBord(lr,lk) and b[lr][lk]==LEEG do
              zetten[#zetten+1] = {lr, lk, cr, ck}
              lr = lr+d[1]; lk = lk+d[2]
            end
          end
          break  -- geblokkeerd
        end
        cr = cr+d[1]; ck = ck+d[2]
      end
    end
  else
    local dirs = {{1,1},{1,-1},{-1,1},{-1,-1}}
    for _, d in ipairs(dirs) do
      local mr, mk = r+d[1], k+d[2]     -- mogelijke tegenstander
      local lr, lk = r+2*d[1], k+2*d[2] -- landingsvak
      if inBord(mr,mk) and inBord(lr,lk) then
        local t = b[mr][mk]
        if kleurVan(t)==-kleur and not uitgesloten[mr..","..mk] and b[lr][lk]==LEEG then
          zetten[#zetten+1] = {lr, lk, mr, mk}
        end
      end
    end
  end
  return zetten
end

-- Zijn er slagzetten voor kleur op bord b?
local function heeftSlagzetten(b, kleur)
  for r=1,10 do
    for k=1,10 do
      if kleurVan(b[r][k]) == kleur then
        if #slagzettenVanaf(b, r, k, {}) > 0 then return true end
      end
    end
  end
  return false
end

-- Alle legale zetten voor een schijf (met slagverplichting)
local function zettenVoorSchijf(b, r, k, slagVerplicht_)
  if slagVerplicht_ then
    return slagzettenVanaf(b, r, k, {})
  else
    return gewoneZetten(b, r, k)
  end
end

-- Alle legale zetten voor kleur
local function alleZettenVoorKleur(b, kleur)
  local slag = heeftSlagzetten(b, kleur)
  local alle = {}
  for r=1,10 do
    for k=1,10 do
      if kleurVan(b[r][k]) == kleur then
        local zetten
        if slag then
          zetten = slagzettenVanaf(b, r, k, {})
        else
          zetten = gewoneZetten(b, r, k)
        end
        for _, z in ipairs(zetten) do
          alle[#alle+1] = {r, k, z[1], z[2], z[3], z[4]}
          -- {vanR, vanK, naarR, naarK, geslagenR, geslagenK}
        end
      end
    end
  end
  return alle
end

-- =============================================================
--  ZET UITVOEREN
-- =============================================================

-- Voer één zet uit op bord b, geef terug of er gepromoveerd werd
local function voerZetUit(b, r0, k0, r1, k1, sr, sk)
  local stuk = b[r0][k0]
  b[r1][k1] = stuk
  b[r0][k0] = LEEG
  local geslagenStuk = LEEG
  if sr then
    geslagenStuk = b[sr][sk]
    b[sr][sk] = LEEG
  end

  -- Promotie: schijf bereikt laatste rij
  if stuk==WIT   and r1==1  then b[r1][k1] = WIT_DAM   end
  if stuk==ZWART and r1==10 then b[r1][k1] = ZWART_DAM end

  return geslagenStuk
end

-- =============================================================
--  EVALUATIE & MINIMAX
-- =============================================================

local function evalueer(b)
  local score = 0
  for r=1,10 do
    for k=1,10 do
      local s = b[r][k]
      if s==WIT       then score = score + 100
      elseif s==ZWART then score = score - 100
      elseif s==WIT_DAM   then score = score + 300
      elseif s==ZWART_DAM then score = score - 300
      end
    end
  end
  return score
end

-- Minimax met alpha-beta, beperkte diepte
-- Meerslaan wordt als één zet beschouwd (vereenvoudiging voor AI)
local function minimax(b, diepte, alfa, beta, maxSpeler, kleur)
  local zetten = alleZettenVoorKleur(b, kleur)
  if diepte==0 or #zetten==0 then
    return evalueer(b), nil
  end

  local beste = nil
  if maxSpeler then
    local best = -99999
    for _, z in ipairs(zetten) do
      local nb = kopieerBord(b)
      voerZetUit(nb, z[1], z[2], z[3], z[4], z[5], z[6])
      local sc = minimax(nb, diepte-1, alfa, beta, false, -kleur)
      if sc > best then best=sc; beste=z end
      if sc > alfa then alfa=sc end
      if beta <= alfa then break end
    end
    return best, beste
  else
    local best = 99999
    for _, z in ipairs(zetten) do
      local nb = kopieerBord(b)
      voerZetUit(nb, z[1], z[2], z[3], z[4], z[5], z[6])
      local sc = minimax(nb, diepte-1, alfa, beta, true, -kleur)
      if sc < best then best=sc; beste=z end
      if sc < beta then beta=sc end
      if beta <= alfa then break end
    end
    return best, beste
  end
end

local function computerZet()
  local diepte = niveauComp==1 and 2 or (niveauComp==2 and 4 or 6)

  if niveauComp==1 then
    local zetten = alleZettenVoorKleur(bord, aanBeurt)
    if #zetten==0 then return nil end
    local kans = bkos.sys.millis() % 3
    if kans ~= 0 then
      local idx = (bkos.sys.millis() % #zetten)+1
      return zetten[idx]
    end
  end

  local maxSpeler = aanBeurt==1
  local _, z = minimax(bord, diepte, -99999, 99999, maxSpeler, aanBeurt)
  return z
end

-- =============================================================
--  ZET TOEPASSEN (op hoofdbord)
-- =============================================================

local function updateStatus()
  local zetten = alleZettenVoorKleur(bord, aanBeurt)
  if #zetten==0 then
    matOfPat = "mat"
    berichtTekst = aanBeurt==1 and "Wit heeft geen zet! Zwart wint." or "Zwart heeft geen zet! Wit wint."
  else
    matOfPat = ""
    berichtTekst = ""
  end
  slagVerplicht = heeftSlagzetten(bord, aanBeurt)
  if slagVerplicht and matOfPat=="" then
    berichtTekst = "Slaan verplicht!"
  end
end

local function pasZetToe(r0, k0, r1, k1, sr, sk)
  local geslagenStuk = voerZetUit(bord, r0, k0, r1, k1, sr, sk)
  if geslagenStuk~=LEEG then
    if isWit(geslagenStuk) then geslagenWit=geslagenWit+1
    else geslagenZwart=geslagenZwart+1 end
  end
  laatstzet = {r0, k0, r1, k1}

  -- Controleer meerslaan (alleen als er geslagen is en de schijf nog kan slaan)
  if sr and #slagzettenVanaf(bord, r1, k1, {}) > 0 then
    meerslaanActief = true
    meerslaanStuk   = {r1, k1}
    geselecteerd    = {r1, k1}
    mogelijkeZetten = slagzettenVanaf(bord, r1, k1, {})
    berichtTekst    = "Meerslaan mogelijk!"
    slagVerplicht   = true
    return
  end

  meerslaanActief = false
  meerslaanStuk   = nil
  geselecteerd    = nil
  mogelijkeZetten = {}
  aanBeurt        = -aanBeurt
  updateStatus()
end

-- =============================================================
--  TEKEN-FUNCTIES
-- =============================================================

local function tekenSchijf(cx, cy, isWitStuk, isDamStuk)
  local radius = 18
  local vul  = isWitStuk and C.witStuk   or C.zwartStuk
  local rand = isWitStuk and C.witRand   or C.zwartRand
  local high = isWitStuk and bkos.color565(255,255,240) or bkos.color565(80,60,50)

  -- Schaduw
  bkos.fillCircle(cx+2, cy+2, radius, bkos.color565(0,0,0))
  -- Hoofd cirkel
  bkos.fillCircle(cx, cy, radius, vul)
  -- Rand
  bkos.drawCircle(cx, cy, radius, rand)
  bkos.drawCircle(cx, cy, radius-1, rand)
  -- Glans (kleine highlight cirkel linksboven)
  bkos.fillCircle(cx-6, cy-6, 5, high)

  if isDamStuk then
    -- Kroon: gouden ster / kruis patroon
    local kc = C.damKroon
    -- Buitenste ring
    bkos.drawCircle(cx, cy, 10, kc)
    bkos.drawCircle(cx, cy,  9, kc)
    -- Kruisje
    bkos.drawFastHLine(cx-8, cy, 17, kc)
    bkos.drawFastVLine(cx, cy-8, 17, kc)
    -- Diagonalen
    bkos.drawLine(cx-6, cy-6, cx+6, cy+6, kc)
    bkos.drawLine(cx+6, cy-6, cx-6, cy+6, kc)
    -- Centraal bolletje
    bkos.fillCircle(cx, cy, 3, kc)
  end
end

local function tekenVakje(r, k)
  local x = BX + (k-1)*SQ
  local y = BY + (r-1)*SQ

  if not isDonkerVak(r,k) then
    bkos.fillRect(x, y, SQ, SQ, C.lichtVak)
    return
  end

  local kleur = C.donkerVak

  -- Laatste zet
  if laatstzet then
    if (r==laatstzet[1] and k==laatstzet[2]) or (r==laatstzet[3] and k==laatstzet[4]) then
      kleur = C.lastMove
    end
  end

  -- Selectie
  if geselecteerd and geselecteerd[1]==r and geselecteerd[2]==k then
    kleur = C.selectie
  end

  -- Mogelijke zetten / slagdoel
  for _, z in ipairs(mogelijkeZetten) do
    if z[1]==r and z[2]==k then
      kleur = slagVerplicht and C.slagpad or C.mogelijkH
    end
  end

  bkos.fillRect(x, y, SQ, SQ, kleur)

  -- Teken schijf
  local stuk = bord[r][k]
  if stuk ~= LEEG then
    local cx = x + SQ//2
    local cy = y + SQ//2
    tekenSchijf(cx, cy, isWit(stuk), isDam(stuk))
  end
end

local function tekenBord()
  bkos.drawRect(BX-2, BY-2, 10*SQ+4, 10*SQ+4, C.border)
  for r=1,10 do
    for k=1,10 do
      tekenVakje(r, k)
    end
  end
  -- Rij-/kolomnummers
  for i=1,10 do
    bkos.drawText(BX+(i-1)*SQ+18, BY+10*SQ+2, tostring(i), 1, C.tekstDim)
    bkos.drawText(BX-10, BY+(i-1)*SQ+18, tostring(i), 1, C.tekstDim)
  end
end

-- ── Knop teken ────────────────────────────────────────────────
local function tekenKnop(x, y, w, h, tekst, actief, maat)
  maat = maat or 2
  local bg = actief and C.knopActi or C.knopInac
  bkos.fillRoundRect(x, y, w, h, 6, bg)
  bkos.drawRoundRect(x, y, w, h, 6, C.border)
  local tx = x + (w - #tekst*(maat*6)) // 2
  local ty = y + (h - maat*8) // 2
  bkos.drawText(tx, ty, tekst, maat, C.knopTek)
end

-- ── Panel ─────────────────────────────────────────────────────
local function tekenPanel()
  local px = 482
  local pw = 308
  bkos.fillRect(px-2, 5, pw, 10*SQ+4, C.panel)
  bkos.drawRect(px-2, 5, pw, 10*SQ+4, C.border)

  -- Aan de beurt indicator
  local beurtTekst = aanBeurt==1 and "Wit aan zet" or "Zwart aan zet"
  local beurtKleur = aanBeurt==1 and C.witStuk or C.tekstDim
  bkos.drawText(px+4, 14, beurtTekst, 2, beurtKleur)

  -- Modus
  local modus = tegenComputer and "Speler vs Computer" or "Speler vs Speler"
  bkos.drawText(px+4, 38, modus, 1, C.tekstDim)
  if tegenComputer then
    local niv = niveauComp==1 and "Makkelijk" or (niveauComp==2 and "Gemiddeld" or "Moeilijk")
    local jouw = spelersKleur==1 and "Jij: Wit" or "Jij: Zwart"
    bkos.drawText(px+4, 50, jouw.."  |  "..niv, 1, C.tekstDim)
  end

  -- Statusbericht
  bkos.drawFastHLine(px, 66, pw-4, C.border)
  if berichtTekst ~= "" then
    local bc = matOfPat~="" and C.check or C.highlight
    bkos.fillRoundRect(px+2, 70, pw-8, 26, 4, C.bg)
    bkos.drawText(px+6, 74, berichtTekst, 1, bc)
  end

  -- Score / geslagen schijven
  bkos.drawFastHLine(px, 102, pw-4, C.border)
  bkos.drawText(px+4, 106, "Geslagen:", 1, C.tekstDim)

  -- Wit geslagen (door zwart)
  bkos.drawText(px+4, 120, "Wit:", 1, C.tekst)
  for i=1, geslagenWit do
    local sx = px+40 + (i-1)*14
    if sx < px+pw-10 then
      bkos.fillCircle(sx, 125, 5, C.witStuk)
      bkos.drawCircle(sx, 125, 5, C.witRand)
    end
  end
  if geslagenWit==0 then bkos.drawText(px+40, 120, "-", 1, C.tekstDim) end

  -- Zwart geslagen (door wit)
  bkos.drawText(px+4, 138, "Zwart:", 1, C.tekst)
  for i=1, geslagenZwart do
    local sx = px+52 + (i-1)*14
    if sx < px+pw-10 then
      bkos.fillCircle(sx, 143, 5, C.zwartStuk)
      bkos.drawCircle(sx, 143, 5, C.zwartRand)
    end
  end
  if geslagenZwart==0 then bkos.drawText(px+52, 138, "-", 1, C.tekstDim) end

  -- Regels uitleg
  bkos.drawFastHLine(px, 158, pw-4, C.border)
  bkos.drawText(px+4, 162, "Regels:", 1, C.tekstDim)
  bkos.drawText(px+4, 174, "Slaan is verplicht", 1, C.tekst)
  bkos.drawText(px+4, 186, "Meerslaan verplicht", 1, C.tekst)
  bkos.drawText(px+4, 198, "Dam: vrij over diag.", 1, C.tekst)
  -- Dam legenda
  bkos.drawFastHLine(px, 214, pw-4, C.border)
  bkos.drawText(px+4, 218, "Legenda:", 1, C.tekstDim)
  -- Wit schijf
  bkos.fillCircle(px+16, 236, 10, C.witStuk)
  bkos.drawCircle(px+16, 236, 10, C.witRand)
  bkos.drawText(px+30, 231, "Witte schijf", 1, C.tekst)
  -- Zwart schijf
  bkos.fillCircle(px+16, 254, 10, C.zwartStuk)
  bkos.drawCircle(px+16, 254, 10, C.zwartRand)
  bkos.drawText(px+30, 249, "Zwarte schijf", 1, C.tekst)
  -- Witte dam
  bkos.fillCircle(px+16, 272, 10, C.witStuk)
  bkos.drawCircle(px+16, 272, 10, C.witRand)
  bkos.drawCircle(px+16, 272, 6, C.damKroon)
  bkos.fillCircle(px+16, 272, 2, C.damKroon)
  bkos.drawText(px+30, 267, "Witte dam", 1, C.tekst)
  -- Zwarte dam
  bkos.fillCircle(px+16, 290, 10, C.zwartStuk)
  bkos.drawCircle(px+16, 290, 10, C.zwartRand)
  bkos.drawCircle(px+16, 290, 6, C.damKroon)
  bkos.fillCircle(px+16, 290, 2, C.damKroon)
  bkos.drawText(px+30, 285, "Zwarte dam", 1, C.tekst)

  -- Knoppen
  bkos.drawFastHLine(px, 310, pw-4, C.border)
  if matOfPat ~= "" then
    tekenKnop(px+4, 318, pw-12, 38, "Nieuw spel", true,  2)
    tekenKnop(px+4, 362, pw-12, 38, "Menu",       false, 2)
  else
    tekenKnop(px+4, 318, pw-12, 38, "Nieuw spel", false, 2)
    tekenKnop(px+4, 362, pw-12, 38, "Menu",       false, 2)
    tekenKnop(px+4, 406, pw-12, 38, "Geef op",    false, 2)
  end
end

-- =============================================================
--  MENU SCHERMEN
-- =============================================================

local function tekenMenu()
  bkos.fillScreen(C.bg)
  bkos.drawText(220, 30,  "DAMSPEL",           4, C.highlight)
  bkos.drawText(220, 76,  "BKOS Boordcomputer", 2, C.tekstDim)
  bkos.drawText(180, 110, "Internationaal damspel - 10x10", 1, C.tekstDim)

  -- Mini bord preview
  local mx, my, ms = 270, 130, 24
  for r=0,9 do
    for k=0,9 do
      local kleur = (r+k)%2==0 and C.lichtVak or C.donkerVak
      bkos.fillRect(mx+k*ms, my+r*ms, ms, ms, kleur)
    end
  end
  bkos.drawRect(mx-1, my-1, 10*ms+2, 10*ms+2, C.border)
  -- Paar schijven op preview
  for r=0,1 do
    for k=0,9 do
      if (r+k)%2==1 then
        bkos.fillCircle(mx+k*ms+ms//2, my+r*ms+ms//2, ms//2-3, C.zwartStuk)
        bkos.drawCircle(mx+k*ms+ms//2, my+r*ms+ms//2, ms//2-3, C.zwartRand)
      end
    end
  end
  for r=8,9 do
    for k=0,9 do
      if (r+k)%2==1 then
        bkos.fillCircle(mx+k*ms+ms//2, my+r*ms+ms//2, ms//2-3, C.witStuk)
        bkos.drawCircle(mx+k*ms+ms//2, my+r*ms+ms//2, ms//2-3, C.witRand)
      end
    end
  end

  tekenKnop(180, 390, 180, 50, "2 Spelers",    false, 2)
  tekenKnop(380, 390, 180, 50, "vs Computer",  false, 2)
end

local function tekenKleurKeuze()
  bkos.fillScreen(C.bg)
  bkos.drawText(220, 40, "Kies uw kleur", 3, C.tekst)

  -- Wit schijf groot
  bkos.fillRoundRect(140, 130, 170, 170, 12, bkos.color565(50,60,80))
  bkos.fillCircle(225, 215, 55, C.witStuk)
  bkos.drawCircle(225, 215, 55, C.witRand)
  bkos.drawCircle(225, 215, 52, C.witRand)
  bkos.fillCircle(205, 195, 16, bkos.color565(255,255,245))
  bkos.drawText(185, 290, "Wit (begint)", 1, C.tekst)

  -- Zwart schijf groot
  bkos.fillRoundRect(480, 130, 170, 170, 12, bkos.color565(50,60,80))
  bkos.fillCircle(565, 215, 55, C.zwartStuk)
  bkos.drawCircle(565, 215, 55, C.zwartRand)
  bkos.drawCircle(565, 215, 52, C.zwartRand)
  bkos.fillCircle(545, 195, 10, bkos.color565(80,60,50))
  bkos.drawText(490, 290, "Zwart", 1, C.tekst)

  tekenKnop(300, 380, 200, 50, "Terug", false, 2)
end

local function tekenNiveauKeuze()
  bkos.fillScreen(C.bg)
  local jouwKleur = spelersKleur==1 and "Wit" or "Zwart"
  bkos.drawText(150, 30, "Moeilijkheidsgraad", 3, C.tekst)
  bkos.drawText(290, 68, "U speelt: "..jouwKleur, 2, C.highlight)

  local namen = {"Makkelijk","Gemiddeld","Moeilijk"}
  local omschr = {
    "Denkt 2 zetten vooruit",
    "Denkt 4 zetten vooruit",
    "Denkt 6 zetten vooruit",
  }
  for i=1,3 do
    tekenKnop(240, 110+(i-1)*90, 320, 60, namen[i], niveauComp==i, 2)
    bkos.drawText(270, 176+(i-1)*90, omschr[i], 1, C.tekstDim)
  end
  tekenKnop(240, 420, 150, 44, "Terug", false, 2)
  tekenKnop(410, 420, 150, 44, "Start!", true, 2)
end

-- =============================================================
--  HOOFD TEKEN CALLBACK
-- =============================================================

bkos.draw = function()
  if staat == MENU   then tekenMenu()
  elseif staat==KLEUR  then tekenKleurKeuze()
  elseif staat==NIVEAU then tekenNiveauKeuze()
  elseif staat==SPEL   then
    bkos.fillScreen(C.bg)
    tekenBord()
    tekenPanel()
  end
end

-- =============================================================
--  BORD INTERACTIE
-- =============================================================

local function bordKlik(r, k)
  if matOfPat ~= "" then return end
  if tegenComputer and aanBeurt ~= spelersKleur then return end
  if not isDonkerVak(r,k) then return end

  local stuk = bord[r][k]

  -- Meerslaan: alleen het actieve stuk mag bewegen
  if meerslaanActief then
    if geselecteerd and geselecteerd[1]==r and geselecteerd[2]==k then
      return  -- klik op eigen stuk, niets doen
    end
    -- Kijk of klik een mogelijke slagzet is
    for _, z in ipairs(mogelijkeZetten) do
      if z[1]==r and z[2]==k then
        pasZetToe(meerslaanStuk[1], meerslaanStuk[2], r, k, z[3], z[4])
        return
      end
    end
    return  -- ongeldige klik tijdens meerslaan
  end

  -- Normaal: selectie of zet
  if geselecteerd then
    -- Kijk of het een geldige zet is
    local gevonden = nil
    for _, z in ipairs(mogelijkeZetten) do
      if z[1]==r and z[2]==k then
        gevonden = z; break
      end
    end

    if gevonden then
      pasZetToe(geselecteerd[1], geselecteerd[2], r, k, gevonden[3], gevonden[4])
    elseif kleurVan(stuk)==aanBeurt then
      -- Selecteer ander stuk
      geselecteerd = {r, k}
      local slag = heeftSlagzetten(bord, aanBeurt)
      mogelijkeZetten = slag and slagzettenVanaf(bord, r, k, {}) or gewoneZetten(bord, r, k)
      -- Als slagverplicht maar dit stuk heeft geen slagzetten, toon leeg
      if slag and #mogelijkeZetten==0 then
        mogelijkeZetten = {}
        berichtTekst = "Dit stuk kan niet slaan!"
      end
    else
      geselecteerd = nil
      mogelijkeZetten = {}
    end
  else
    if kleurVan(stuk)==aanBeurt then
      geselecteerd = {r, k}
      local slag = heeftSlagzetten(bord, aanBeurt)
      mogelijkeZetten = slag and slagzettenVanaf(bord, r, k, {}) or gewoneZetten(bord, r, k)
      if slag and #mogelijkeZetten==0 then
        mogelijkeZetten = {}
        berichtTekst = "Dit stuk kan niet slaan!"
      end
    end
  end
end

-- =============================================================
--  UPDATE: COMPUTER BEURT
-- =============================================================

bkos.update = function()
  if staat ~= SPEL then return end
  if matOfPat ~= "" then return end
  if not tegenComputer then return end
  if aanBeurt == spelersKleur then return end
  if meerslaanActief then return end  -- computer meerslaan wordt intern afgehandeld

  local z = computerZet()
  if z then
    pasZetToe(z[1], z[2], z[3], z[4], z[5], z[6])
    -- Computer meerslaan
    local veilig = 0
    while meerslaanActief and veilig < 20 do
      veilig = veilig + 1
      local zetten = slagzettenVanaf(bord, meerslaanStuk[1], meerslaanStuk[2], {})
      if #zetten == 0 then break end
      local idx = (bkos.sys.millis() % #zetten)+1
      local mz = zetten[idx]
      pasZetToe(meerslaanStuk[1], meerslaanStuk[2], mz[1], mz[2], mz[3], mz[4])
    end
  end
end

-- =============================================================
--  TOUCH CALLBACK
-- =============================================================

bkos.touch = function(x, y)

  if staat == MENU then
    if x>=180 and x<=360 and y>=390 and y<=440 then
      tegenComputer = false
      initialiseerBord()
      updateStatus()
      staat = SPEL
    end
    if x>=380 and x<=560 and y>=390 and y<=440 then
      tegenComputer = true
      staat = KLEUR
    end

  elseif staat == KLEUR then
    if x>=140 and x<=310 and y>=130 and y<=300 then
      spelersKleur = 1; staat = NIVEAU
    end
    if x>=480 and x<=650 and y>=130 and y<=300 then
      spelersKleur = -1; staat = NIVEAU
    end
    if x>=300 and x<=500 and y>=380 and y<=430 then
      staat = MENU
    end

  elseif staat == NIVEAU then
    if x>=240 and x<=560 and y>=110 and y<=170 then niveauComp=1 end
    if x>=240 and x<=560 and y>=200 and y<=260 then niveauComp=2 end
    if x>=240 and x<=560 and y>=290 and y<=350 then niveauComp=3 end
    if x>=240 and x<=390 and y>=420 and y<=464 then staat=KLEUR end
    if x>=410 and x<=560 and y>=420 and y<=464 then
      initialiseerBord()
      updateStatus()
      staat = SPEL
    end

  elseif staat == SPEL then
    local px = 480
    local pw = 310
    -- Knoppen panel
    if x>=px and x<=px+pw and y>=318 and y<=356 then
      initialiseerBord(); updateStatus(); return
    end
    if x>=px and x<=px+pw and y>=362 and y<=400 then
      staat=MENU; return
    end
    if x>=px and x<=px+pw and y>=406 and y<=444 then
      berichtTekst = aanBeurt==1 and "Wit geeft op! Zwart wint." or "Zwart geeft op! Wit wint."
      matOfPat = "mat"; return
    end

    -- Bord klik
    if x>=BX and x<BX+10*SQ and y>=BY and y<BY+10*SQ then
      local k = math.floor((x-BX)/SQ)+1
      local r = math.floor((y-BY)/SQ)+1
      bordKlik(r, k)
    end
  end
end

-- =============================================================
--  INIT
-- =============================================================
staat = MENU

-- ============================================================
--  SCHAAKSPEL  v1.0  –  BKOS Lua App API v2
--  800 × 480 designresolutie
--  Scherm-layout:
--    Bord  : x=20,  y=10,  8×8 vakjes van 56×56 = 448×448
--    Panel : x=490, y=10,  breedte ~290
-- ============================================================

-- ── Kleurpalet ──────────────────────────────────────────────
local C = {
  bg       = bkos.color565( 18,  24,  38),
  panel    = bkos.color565( 28,  36,  54),
  border   = bkos.color565( 60,  80, 120),
  lichtVak = bkos.color565(240, 217, 181),
  donkerVak= bkos.color565(181, 136,  99),
  selectie = bkos.color565(100, 200,  80),
  mogelijkH= bkos.color565( 80, 180, 240),
  check    = bkos.color565(220,  60,  40),
  wit      = bkos.color565(250, 250, 250),
  zwart    = bkos.color565( 20,  20,  20),
  tekst    = bkos.color565(220, 220, 235),
  tekstDim = bkos.color565(120, 130, 160),
  knopActi = bkos.color565( 60, 140, 220),
  knopInac = bkos.color565( 40,  55,  85),
  knopTek  = bkos.color565(240, 245, 255),
  highlight= bkos.color565(255, 210,  50),
  lastMove = bkos.color565(180, 160,  40),
}

-- ── Bord-layout ──────────────────────────────────────────────
local BX  = 20   -- bord linker x
local BY  = 10   -- bord boven y
local SQ  = 56   -- vakgrootte

-- ── Stukken: positief = wit, negatief = zwart ────────────────
-- 1=Pion 2=Toren 3=Paard 4=Loper 5=Koningin 6=Koning
local PION=1; local TOREN=2; local PAARD=3
local LOPER=4; local KONINGIN=5; local KONING=6

-- ── Speelstatus ───────────────────────────────────────────────
local MENU   = 0
local KLEUR  = 1
local NIVEAU = 2
local SPEL   = 3
local PROMO  = 4

-- ── Staat ────────────────────────────────────────────────────
local staat = MENU

local bord = {}            -- bord[rij][kol] = stukwaarde
local aanBeurt = 1         -- 1=wit, -1=zwart
local geselecteerd = nil   -- {r,k}
local mogelijkeZetten = {} -- lijst van {r,k}
local berichtTekst = ""
local laatstzet = nil      -- {r0,k0,r1,k1}

-- ── Spelmodus ────────────────────────────────────────────────
local tegenComputer = false
local spelersKleur  = 1    -- 1=wit, -1=zwart (voor speler vs computer)
local niveauComp    = 2    -- 1=makkelijk 2=gemiddeld 3=moeilijk

-- ── Rokade-rechten ───────────────────────────────────────────
local rokadeRechten = {
  witKoning  = true,
  witTorenK  = true,  -- koningszijde
  witTorenD  = true,  -- damezijde
  zwartKoning  = true,
  zwartTorenK  = true,
  zwartTorenD  = true,
}

-- ── En-passant ───────────────────────────────────────────────
local enPassantKolom = nil  -- kolom waar en-passant mogelijk is
local enPassantRij   = nil

-- ── Promotie ─────────────────────────────────────────────────
local promoZet  = nil   -- {r0,k0,r1,k1,kleur}

-- ── Schaak-vlag ──────────────────────────────────────────────
local inSchaak = false
local matOfPat = ""   -- "mat" | "pat" | ""

-- =============================================================
--  HULPFUNCTIES
-- =============================================================

local function inBord(r,k)
  return r>=1 and r<=8 and k>=1 and k<=8
end

local function kleurVan(stuk)
  if stuk > 0 then return 1
  elseif stuk < 0 then return -1
  else return 0 end
end

local function absStuk(stuk)
  return stuk < 0 and -stuk or stuk
end

-- Kopieer bord (voor simulatie)
local function kopieerBord(b)
  local n = {}
  for r=1,8 do
    n[r] = {}
    for k=1,8 do n[r][k] = b[r][k] end
  end
  return n
end

-- Kopieer rokadeRechten
local function kopieerRokade(rr)
  local n = {}
  for k,v in pairs(rr) do n[k]=v end
  return n
end

-- =============================================================
--  STARTPOSITIE
-- =============================================================

local function initialiseerBord()
  bord = {}
  for r=1,8 do
    bord[r]={}
    for k=1,8 do bord[r][k]=0 end
  end
  -- Zwarte stukken (rij 8 en 7)
  bord[8][1]=-TOREN; bord[8][2]=-PAARD; bord[8][3]=-LOPER; bord[8][4]=-KONINGIN
  bord[8][5]=-KONING; bord[8][6]=-LOPER; bord[8][7]=-PAARD; bord[8][8]=-TOREN
  for k=1,8 do bord[7][k]=-PION end
  -- Witte stukken (rij 1 en 2)
  bord[1][1]=TOREN; bord[1][2]=PAARD; bord[1][3]=LOPER; bord[1][4]=KONINGIN
  bord[1][5]=KONING; bord[1][6]=LOPER; bord[1][7]=PAARD; bord[1][8]=TOREN
  for k=1,8 do bord[2][k]=PION end

  aanBeurt = 1
  geselecteerd = nil
  mogelijkeZetten = {}
  berichtTekst = ""
  laatstzet = nil
  enPassantKolom = nil
  enPassantRij   = nil
  inSchaak = false
  matOfPat = ""
  rokadeRechten = {
    witKoning=true, witTorenK=true, witTorenD=true,
    zwartKoning=true, zwartTorenK=true, zwartTorenD=true,
  }
end

-- =============================================================
--  ZETGENERATIE  (pseudo-legale zetten, zonder schaakcontrole)
-- =============================================================

local function zettenVanStuk(b, r, k, kleur, ep_k, ep_r, rr)
  local zetten = {}
  local stuk = absStuk(b[r][k])
  if stuk == 0 then return zetten end

  local function voegToe(dr, dk)
    if inBord(dr,dk) then
      local doel = b[dr][dk]
      if kleurVan(doel) ~= kleur then
        zetten[#zetten+1] = {dr, dk}
        return kleurVan(doel) == 0  -- kan verder schuiven als leeg
      end
    end
    return false
  end

  local function schuif(dr, dk)
    local cr, ck = r+dr, k+dk
    while inBord(cr,ck) do
      local doel = b[cr][ck]
      if kleurVan(doel) == kleur then break end
      zetten[#zetten+1] = {cr, ck}
      if doel ~= 0 then break end  -- geslagen, stop
      cr = cr+dr; ck = ck+dk
    end
  end

  if stuk == PION then
    local richting = kleur  -- wit gaat omhoog (rij+1), zwart omlaag (rij-1)
    -- 1 stap
    if inBord(r+richting, k) and b[r+richting][k] == 0 then
      zetten[#zetten+1] = {r+richting, k}
      -- 2 stap vanuit startpositie
      local startRij = kleur==1 and 2 or 7
      if r == startRij and b[r+2*richting][k] == 0 then
        zetten[#zetten+1] = {r+2*richting, k}
      end
    end
    -- Slaan diagonaal
    for _, dk in ipairs({-1, 1}) do
      local nr, nk = r+richting, k+dk
      if inBord(nr,nk) then
        if kleurVan(b[nr][nk]) == -kleur then
          zetten[#zetten+1] = {nr, nk}
        end
        -- En-passant
        if ep_k == nk and ep_r == nr then
          zetten[#zetten+1] = {nr, nk, "ep"}
        end
      end
    end

  elseif stuk == TOREN then
    schuif(1,0); schuif(-1,0); schuif(0,1); schuif(0,-1)

  elseif stuk == PAARD then
    local sprongen = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}}
    for _, s in ipairs(sprongen) do voegToe(r+s[1], k+s[2]) end

  elseif stuk == LOPER then
    schuif(1,1); schuif(1,-1); schuif(-1,1); schuif(-1,-1)

  elseif stuk == KONINGIN then
    schuif(1,0); schuif(-1,0); schuif(0,1); schuif(0,-1)
    schuif(1,1); schuif(1,-1); schuif(-1,1); schuif(-1,-1)

  elseif stuk == KONING then
    local dirs = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}}
    for _, d in ipairs(dirs) do voegToe(r+d[1], k+d[2]) end
    -- Rokade (alleen als rr opgegeven)
    if rr then
      local rijK = kleur==1 and 1 or 8
      if r == rijK and k == 5 then
        -- Koningszijde
        local kkr = kleur==1 and "witTorenK" or "zwartTorenK"
        local kgr = kleur==1 and "witKoning" or "zwartKoning"
        if rr[kgr] and rr[kkr] and b[rijK][6]==0 and b[rijK][7]==0 then
          zetten[#zetten+1] = {rijK, 7, "rokK"}
        end
        -- Damezijde
        local dkr = kleur==1 and "witTorenD" or "zwartTorenD"
        if rr[kgr] and rr[dkr] and b[rijK][4]==0 and b[rijK][3]==0 and b[rijK][2]==0 then
          zetten[#zetten+1] = {rijK, 3, "rokD"}
        end
      end
    end
  end

  return zetten
end

-- =============================================================
--  SCHAAKDETECTIE
-- =============================================================

local function isVeldAangevallen(b, r, k, doorKleur)
  -- Controleert of veld (r,k) aangevallen wordt door 'doorKleur'
  for br=1,8 do
    for bk=1,8 do
      if kleurVan(b[br][bk]) == doorKleur then
        local zetten = zettenVanStuk(b, br, bk, doorKleur, nil, nil, nil)
        for _, z in ipairs(zetten) do
          if z[1]==r and z[2]==k then return true end
        end
      end
    end
  end
  return false
end

local function koningPositie(b, kleur)
  for r=1,8 do
    for k=1,8 do
      if b[r][k] == kleur * KONING then return r, k end
    end
  end
  return nil, nil
end

local function isInSchaak(b, kleur)
  local r, k = koningPositie(b, kleur)
  if not r then return false end
  return isVeldAangevallen(b, r, k, -kleur)
end

-- =============================================================
--  ZET UITVOEREN (op een bordkopie)
-- =============================================================

local function voerZetUit(b, r0, k0, r1, k1, extra, rr, promo)
  local stuk = b[r0][k0]
  local kleur = kleurVan(stuk)
  b[r1][k1] = promo and (kleur * promo) or stuk
  b[r0][k0] = 0

  -- En-passant: sla de pion naast
  if extra == "ep" then
    b[r0][k1] = 0
  end

  -- Rokade: beweeg toren mee
  if extra == "rokK" then
    local rijK = kleur==1 and 1 or 8
    b[rijK][6] = b[rijK][8]
    b[rijK][8] = 0
  elseif extra == "rokD" then
    local rijK = kleur==1 and 1 or 8
    b[rijK][4] = b[rijK][1]
    b[rijK][1] = 0
  end

  -- Update rokaderechten
  if rr then
    local abs0 = absStuk(stuk)
    if abs0 == KONING then
      if kleur==1 then rr.witKoning=false; rr.witTorenK=false; rr.witTorenD=false
      else rr.zwartKoning=false; rr.zwartTorenK=false; rr.zwartTorenD=false end
    elseif abs0 == TOREN then
      if kleur==1 then
        if k0==8 then rr.witTorenK=false end
        if k0==1 then rr.witTorenD=false end
      else
        if k0==8 then rr.zwartTorenK=false end
        if k0==1 then rr.zwartTorenD=false end
      end
    end
    -- Als toren geslagen wordt
    if r1==1 and k1==1 then rr.witTorenD=false end
    if r1==1 and k1==8 then rr.witTorenK=false end
    if r1==8 and k1==1 then rr.zwartTorenD=false end
    if r1==8 and k1==8 then rr.zwartTorenK=false end
  end
end

-- =============================================================
--  LEGALE ZETTEN (filter pseudo-legaal op eigen schaak)
-- =============================================================

local function legaleZetten(b, r, k, kleur, ep_k, ep_r, rr)
  local pseudo = zettenVanStuk(b, r, k, kleur, ep_k, ep_r, rr)
  local legaal = {}
  for _, z in ipairs(pseudo) do
    -- Rokade: controleer of tussenliggende velden aangevallen zijn
    if z[3] == "rokK" or z[3] == "rokD" then
      local rijK = kleur==1 and 1 or 8
      local stap = z[3]=="rokK" and 1 or -1
      local vrij = true
      -- Mag koning-startpositie niet in schaak staan
      if isVeldAangevallen(b, rijK, 5, -kleur) then vrij=false end
      if vrij then
        -- Doorgangsfeld
        local nb = kopieerBord(b)
        nb[rijK][5+stap] = nb[rijK][5]
        nb[rijK][5] = 0
        if isVeldAangevallen(nb, rijK, 5+stap, -kleur) then vrij=false end
      end
      if vrij then
        local nb2 = kopieerBord(b)
        local rr2 = kopieerRokade(rr or rokadeRechten)
        voerZetUit(nb2, r, k, z[1], z[2], z[3], rr2, nil)
        if not isInSchaak(nb2, kleur) then
          legaal[#legaal+1] = z
        end
      end
    else
      local nb = kopieerBord(b)
      local rr2 = kopieerRokade(rr or rokadeRechten)
      voerZetUit(nb, r, k, z[1], z[2], z[3], rr2, nil)
      if not isInSchaak(nb, kleur) then
        legaal[#legaal+1] = z
      end
    end
  end
  return legaal
end

local function alleLegaleZetten(b, kleur, ep_k, ep_r, rr)
  local alle = {}
  for r=1,8 do
    for k=1,8 do
      if kleurVan(b[r][k]) == kleur then
        local zetten = legaleZetten(b, r, k, kleur, ep_k, ep_r, rr)
        for _, z in ipairs(zetten) do
          alle[#alle+1] = {r, k, z[1], z[2], z[3]}
        end
      end
    end
  end
  return alle
end

-- =============================================================
--  STUKWAARDEN EN EVALUATIE
-- =============================================================

local STUKWAARDEN = {[PION]=100, [PAARD]=320, [LOPER]=330, [TOREN]=500, [KONINGIN]=900, [KONING]=20000}

-- Eenvoudige positiebonus (middencontrole) voor wit
local PION_TABEL = {
  0,  0,  0,  0,  0,  0,  0,  0,
  50, 50, 50, 50, 50, 50, 50, 50,
  10, 10, 20, 30, 30, 20, 10, 10,
   5,  5, 10, 25, 25, 10,  5,  5,
   0,  0,  0, 20, 20,  0,  0,  0,
   5, -5,-10,  0,  0,-10, -5,  5,
   5, 10, 10,-20,-20, 10, 10,  5,
   0,  0,  0,  0,  0,  0,  0,  0,
}
local PAARD_TABEL = {
  -50,-40,-30,-30,-30,-30,-40,-50,
  -40,-20,  0,  0,  0,  0,-20,-40,
  -30,  0, 10, 15, 15, 10,  0,-30,
  -30,  5, 15, 20, 20, 15,  5,-30,
  -30,  0, 15, 20, 20, 15,  0,-30,
  -30,  5, 10, 15, 15, 10,  5,-30,
  -40,-20,  0,  5,  5,  0,-20,-40,
  -50,-40,-30,-30,-30,-30,-40,-50,
}

local function positieBonus(stuk, r, k, kleur)
  local idx = kleur==1 and ((8-r)*8+k) or ((r-1)*8+(9-k))
  if stuk==PION   then return PION_TABEL[idx]   or 0 end
  if stuk==PAARD  then return PAARD_TABEL[idx]  or 0 end
  return 0
end

local function evalueer(b)
  local score = 0
  for r=1,8 do
    for k=1,8 do
      local stuk = b[r][k]
      if stuk ~= 0 then
        local kleur = kleurVan(stuk)
        local abs   = absStuk(stuk)
        local waarde= (STUKWAARDEN[abs] or 0) + positieBonus(abs, r, k, kleur)
        score = score + kleur * waarde
      end
    end
  end
  return score
end

-- =============================================================
--  MINIMAX MET ALPHA-BETA
-- =============================================================

local function minimax(b, diepte, alfa, beta, maxSpeler, kleur, ep_k, ep_r, rr)
  local zetten = alleLegaleZetten(b, kleur, ep_k, ep_r, rr)
  if diepte == 0 or #zetten == 0 then
    return evalueer(b), nil
  end

  -- Eenvoudig willekeurig mengen voor variatie (zonder math.random seed issues)
  -- Gebruik een deterministische shuffle gebaseerd op bordinhoud
  local beste_zet = nil

  if maxSpeler then
    local maxScore = -99999
    for _, z in ipairs(zetten) do
      local nb  = kopieerBord(b)
      local nrr = kopieerRokade(rr)
      local nieuwEP_k, nieuwEP_r = nil, nil
      -- Detecteer dubbele pionzet voor en-passant
      if absStuk(nb[z[1]][z[2]])==PION and math.abs(z[3]-z[1])==2 then
        nieuwEP_k = z[2]
        nieuwEP_r = z[1] + kleur  -- rij na de pion
      end
      voerZetUit(nb, z[1], z[2], z[3], z[4], z[5], nrr, nil)
      local sc = minimax(nb, diepte-1, alfa, beta, false, -kleur, nieuwEP_k, nieuwEP_r, nrr)
      if sc > maxScore then
        maxScore = sc
        beste_zet = z
      end
      if sc > alfa then alfa = sc end
      if beta <= alfa then break end
    end
    return maxScore, beste_zet
  else
    local minScore = 99999
    for _, z in ipairs(zetten) do
      local nb  = kopieerBord(b)
      local nrr = kopieerRokade(rr)
      local nieuwEP_k, nieuwEP_r = nil, nil
      if absStuk(nb[z[1]][z[2]])==PION and math.abs(z[3]-z[1])==2 then
        nieuwEP_k = z[2]
        nieuwEP_r = z[1] + kleur
      end
      voerZetUit(nb, z[1], z[2], z[3], z[4], z[5], nrr, nil)
      local sc = minimax(nb, diepte-1, alfa, beta, true, -kleur, nieuwEP_k, nieuwEP_r, nrr)
      if sc < minScore then
        minScore = sc
        beste_zet = z
      end
      if sc < beta then beta = sc end
      if beta <= alfa then break end
    end
    return minScore, beste_zet
  end
end

local function computerZet()
  local diepte = niveauComp == 1 and 1 or (niveauComp == 2 and 2 or 3)

  -- Op makkelijk: kies soms willekeurig
  if niveauComp == 1 then
    local zetten = alleLegaleZetten(bord, aanBeurt, enPassantKolom, enPassantRij, rokadeRechten)
    if #zetten == 0 then return end
    -- Mix van beste en willekeurig
    local kans = bkos.sys.millis() % 3
    if kans == 0 then
      -- willekeurige zet
      local idx = (bkos.sys.millis() % #zetten) + 1
      local z = zetten[idx]
      return z
    end
  end

  local maxSpeler = aanBeurt == 1
  local _, z = minimax(bord, diepte, -99999, 99999, maxSpeler, aanBeurt, enPassantKolom, enPassantRij, rokadeRechten)
  return z
end

-- =============================================================
--  ZET TOEPASSEN (op hoofdbord)
-- =============================================================

local function pasZetToe(r0, k0, r1, k1, extra, promo)
  -- Detecteer en-passant situatie voor volgende zet
  local nieuwEP_k, nieuwEP_r = nil, nil
  local stuk = bord[r0][k0]
  if absStuk(stuk)==PION and math.abs(r1-r0)==2 then
    nieuwEP_k = k0
    nieuwEP_r = r0 + aanBeurt
  end

  voerZetUit(bord, r0, k0, r1, k1, extra, rokadeRechten, promo)
  laatstzet = {r0, k0, r1, k1}
  enPassantKolom = nieuwEP_k
  enPassantRij   = nieuwEP_r
  aanBeurt = -aanBeurt

  -- Schaakcontrole
  inSchaak = isInSchaak(bord, aanBeurt)
  local alleZetten = alleLegaleZetten(bord, aanBeurt, enPassantKolom, enPassantRij, rokadeRechten)
  if #alleZetten == 0 then
    if inSchaak then
      matOfPat = "mat"
      berichtTekst = aanBeurt==1 and "Schaakmat! Zwart wint." or "Schaakmat! Wit wint."
    else
      matOfPat = "pat"
      berichtTekst = "Patstelling! Remise."
    end
  elseif inSchaak then
    berichtTekst = "Schaak!"
  else
    berichtTekst = ""
  end
end

-- =============================================================
--  TEKEN-FUNCTIES
-- =============================================================

local function tekenVakje(r, k)
  local x = BX + (k-1)*SQ
  local y = BY + (8-r)*SQ
  local lichtDonker = (r+k)%2==0
  local kleur = lichtDonker and C.lichtVak or C.donkerVak

  -- Laatste zet highlight
  if laatstzet then
    if (r==laatstzet[1] and k==laatstzet[2]) or (r==laatstzet[3] and k==laatstzet[4]) then
      kleur = C.lastMove
    end
  end

  -- Selectie
  if geselecteerd and geselecteerd[1]==r and geselecteerd[2]==k then
    kleur = C.selectie
  end

  -- Mogelijke zetten
  for _, z in ipairs(mogelijkeZetten) do
    if z[1]==r and z[2]==k then
      kleur = C.mogelijkH
    end
  end

  -- Schaak: koningsvak rood
  if inSchaak then
    local kr, kk = koningPositie(bord, aanBeurt)
    if r==kr and k==kk then kleur = C.check end
  end

  bkos.fillRect(x, y, SQ, SQ, kleur)
end

-- =============================================================
--  2D STUK-ILLUSTRATIES
--  ox,oy = linkerbovenhoek van het vakje (56×56)
--  vul   = vulkleur (wit of zwart stuk)
--  rand  = randkleur (contrasterend)
-- =============================================================

local function tekenPion(ox, oy, vul, rand)
  -- Voet: breed rechthoek onderaan
  bkos.fillRoundRect(ox+14, oy+43, 28, 8,  3, vul)
  bkos.drawRoundRect(ox+14, oy+43, 28, 8,  3, rand)
  -- Steel: smal
  bkos.fillRect(ox+23, oy+34, 10, 11, vul)
  bkos.drawRect(ox+23, oy+34, 10, 11, rand)
  -- Hoofd: cirkel
  bkos.fillCircle(ox+28, oy+26, 11, vul)
  bkos.drawCircle(ox+28, oy+26, 11, rand)
  -- Binnenring highlight
  bkos.drawCircle(ox+28, oy+26,  7, rand)
end

local function tekenToren(ox, oy, vul, rand)
  -- Voet
  bkos.fillRoundRect(ox+10, oy+44, 36, 8,  2, vul)
  bkos.drawRoundRect(ox+10, oy+44, 36, 8,  2, rand)
  -- Schacht
  bkos.fillRect(ox+15, oy+22, 26, 24, vul)
  bkos.drawRect(ox+15, oy+22, 26, 24, rand)
  -- Kantelen (3 blokjes bovenop)
  bkos.fillRect(ox+13, oy+14, 8, 10, vul)
  bkos.drawRect(ox+13, oy+14, 8, 10, rand)
  bkos.fillRect(ox+24, oy+14, 8, 10, vul)
  bkos.drawRect(ox+24, oy+14, 8, 10, rand)
  bkos.fillRect(ox+35, oy+14, 8, 10, vul)
  bkos.drawRect(ox+35, oy+14, 8, 10, rand)
  -- Spleten tussen kantelen
  bkos.fillRect(ox+21, oy+14, 3, 10, rand)
  bkos.fillRect(ox+33, oy+14, 2, 10, rand)
end

local function tekenPaard(ox, oy, vul, rand)
  -- Voet
  bkos.fillRoundRect(ox+11, oy+44, 32, 8,  2, vul)
  bkos.drawRoundRect(ox+11, oy+44, 32, 8,  2, rand)
  -- Lichaam (schuin naar voor)
  bkos.fillTriangle(ox+18, oy+44, ox+38, oy+44, ox+32, oy+22, vul)
  bkos.fillTriangle(ox+18, oy+44, ox+25, oy+22, ox+32, oy+22, vul)
  bkos.drawTriangle(ox+18, oy+44, ox+38, oy+44, ox+32, oy+22, rand)
  -- Nek
  bkos.fillRect(ox+20, oy+18, 14, 26, vul)
  bkos.drawRect(ox+20, oy+18, 14, 26, rand)
  -- Hoofd (naar rechts gericht)
  bkos.fillRoundRect(ox+22, oy+12, 22, 14, 4, vul)
  bkos.drawRoundRect(ox+22, oy+12, 22, 14, 4, rand)
  -- Snuit
  bkos.fillRoundRect(ox+36, oy+18, 10, 9,  3, vul)
  bkos.drawRoundRect(ox+36, oy+18, 10, 9,  3, rand)
  -- Oog
  bkos.fillCircle(ox+38, oy+15, 2, rand)
  -- Oor
  bkos.fillTriangle(ox+24, oy+12, ox+28, oy+12, ox+26, oy+7, vul)
  bkos.drawTriangle(ox+24, oy+12, ox+28, oy+12, ox+26, oy+7, rand)
end

local function tekenLoper(ox, oy, vul, rand)
  -- Voet
  bkos.fillRoundRect(ox+12, oy+44, 32, 8,  3, vul)
  bkos.drawRoundRect(ox+12, oy+44, 32, 8,  3, rand)
  -- Basis
  bkos.fillRoundRect(ox+16, oy+37, 24, 9,  2, vul)
  bkos.drawRoundRect(ox+16, oy+37, 24, 9,  2, rand)
  -- Romp (breed naar smal)
  bkos.fillTriangle(ox+16, oy+37, ox+40, oy+37, ox+28, oy+18, vul)
  bkos.drawTriangle(ox+16, oy+37, ox+40, oy+37, ox+28, oy+18, rand)
  -- Hoofd: kleine bol
  bkos.fillCircle(ox+28, oy+16, 8, vul)
  bkos.drawCircle(ox+28, oy+16, 8, rand)
  -- Kruis bovenop
  bkos.fillCircle(ox+28, oy+9,  3, vul)
  bkos.drawCircle(ox+28, oy+9,  3, rand)
  bkos.drawFastVLine(ox+28, oy+6,  6, rand)
  bkos.drawFastHLine(ox+25, oy+9,  6, rand)
end

local function tekenKoningin(ox, oy, vul, rand)
  -- Voet
  bkos.fillRoundRect(ox+9,  oy+44, 38, 8,  3, vul)
  bkos.drawRoundRect(ox+9,  oy+44, 38, 8,  3, rand)
  -- Rok (trapezium)
  bkos.fillTriangle(ox+12, oy+42, ox+44, oy+42, ox+28, oy+24, vul)
  bkos.fillTriangle(ox+14, oy+42, ox+42, oy+42, ox+28, oy+30, vul)
  bkos.drawTriangle(ox+12, oy+42, ox+44, oy+42, ox+28, oy+24, rand)
  -- Lichaam midden
  bkos.fillRoundRect(ox+20, oy+23, 16, 20, 2, vul)
  bkos.drawRoundRect(ox+20, oy+23, 16, 20, 2, rand)
  -- Hoofd
  bkos.fillCircle(ox+28, oy+19, 8, vul)
  bkos.drawCircle(ox+28, oy+19, 8, rand)
  -- Kroon: 3 punten
  bkos.fillTriangle(ox+19, oy+14, ox+23, oy+14, ox+21, oy+8,  vul)
  bkos.drawTriangle(ox+19, oy+14, ox+23, oy+14, ox+21, oy+8,  rand)
  bkos.fillTriangle(ox+26, oy+13, ox+30, oy+13, ox+28, oy+6,  vul)
  bkos.drawTriangle(ox+26, oy+13, ox+30, oy+13, ox+28, oy+6,  rand)
  bkos.fillTriangle(ox+33, oy+14, ox+37, oy+14, ox+35, oy+8,  vul)
  bkos.drawTriangle(ox+33, oy+14, ox+37, oy+14, ox+35, oy+8,  rand)
  -- Kroontjes bolletjes
  bkos.fillCircle(ox+21, oy+8,  2, vul)
  bkos.drawCircle(ox+21, oy+8,  2, rand)
  bkos.fillCircle(ox+28, oy+6,  2, vul)
  bkos.drawCircle(ox+28, oy+6,  2, rand)
  bkos.fillCircle(ox+35, oy+8,  2, vul)
  bkos.drawCircle(ox+35, oy+8,  2, rand)
end

local function tekenKoning(ox, oy, vul, rand)
  -- Voet
  bkos.fillRoundRect(ox+9,  oy+44, 38, 8,  3, vul)
  bkos.drawRoundRect(ox+9,  oy+44, 38, 8,  3, rand)
  -- Rok
  bkos.fillTriangle(ox+12, oy+42, ox+44, oy+42, ox+28, oy+26, vul)
  bkos.drawTriangle(ox+12, oy+42, ox+44, oy+42, ox+28, oy+26, rand)
  -- Lichaam
  bkos.fillRoundRect(ox+20, oy+25, 16, 18, 2, vul)
  bkos.drawRoundRect(ox+20, oy+25, 16, 18, 2, rand)
  -- Hoofd
  bkos.fillCircle(ox+28, oy+20, 8, vul)
  bkos.drawCircle(ox+28, oy+20, 8, rand)
  -- Kruis (groot kruis bovenop)
  bkos.fillRect(ox+26, oy+6,  4, 14, vul)
  bkos.drawRect(ox+26, oy+6,  4, 14, rand)
  bkos.fillRect(ox+21, oy+9,  14, 4,  vul)
  bkos.drawRect(ox+21, oy+9,  14, 4,  rand)
end

local TEKEN_FN = {
  [PION]     = tekenPion,
  [TOREN]    = tekenToren,
  [PAARD]    = tekenPaard,
  [LOPER]    = tekenLoper,
  [KONINGIN] = tekenKoningin,
  [KONING]   = tekenKoning,
}

local function tekenStuk(r, k, stuk)
  if stuk == 0 then return end
  local ox = BX + (k-1)*SQ
  local oy = BY + (8-r)*SQ
  local isWit = stuk > 0
  local vul  = isWit and C.wit  or C.zwart
  local rand = isWit and C.zwart or C.wit
  local fn = TEKEN_FN[absStuk(stuk)]
  if fn then fn(ox, oy, vul, rand) end
end

local function tekenCoordinaten()
  local letters = {"a","b","c","d","e","f","g","h"}
  for k=1,8 do
    local x = BX + (k-1)*SQ + 22
    bkos.drawText(x, BY+8*SQ+2, letters[k], 1, C.tekstDim)
  end
  for r=1,8 do
    local y = BY + (8-r)*SQ + 22
    bkos.drawText(BX-12, y, tostring(r), 1, C.tekstDim)
  end
end

local function tekenBord()
  -- Buitenrand
  bkos.drawRect(BX-2, BY-2, 8*SQ+4, 8*SQ+4, C.border)
  -- Vakjes en stukken
  for r=1,8 do
    for k=1,8 do
      tekenVakje(r, k)
      tekenStuk(r, k, bord[r][k])
    end
  end
  tekenCoordinaten()
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

-- ── Panel tekst ───────────────────────────────────────────────
local function tekenPanel()
  local px = 492
  -- Achtergrond
  bkos.fillRect(px-4, 8, 308, 8*SQ+4, C.panel)
  bkos.drawRect(px-4, 8, 308, 8*SQ+4, C.border)

  -- Aan de beurt
  local beurtTekst = aanBeurt==1 and "Wit aan zet" or "Zwart aan zet"
  local beurtKleur = aanBeurt==1 and C.wit or C.tekstDim
  bkos.drawText(px, 18, beurtTekst, 2, beurtKleur)

  -- Modus info
  local modus = tegenComputer and "Speler vs Computer" or "Speler vs Speler"
  bkos.drawText(px, 42, modus, 1, C.tekstDim)

  if tegenComputer then
    local niv = niveauComp==1 and "Makkelijk" or (niveauComp==2 and "Gemiddeld" or "Moeilijk")
    local jouwKleur = spelersKleur==1 and "Jij: Wit" or "Jij: Zwart"
    bkos.drawText(px, 56, jouwKleur.."  |  "..niv, 1, C.tekstDim)
  end

  -- Bericht / status
  if berichtTekst ~= "" then
    local bc = matOfPat~="" and C.check or C.highlight
    bkos.fillRoundRect(px, 75, 290, 28, 4, C.bg)
    bkos.drawText(px+4, 80, berichtTekst, 2, bc)
  end

  -- Legenda stukken (mini-icoontjes geschaald)
  bkos.drawFastHLine(px, 112, 285, C.border)
  bkos.drawText(px, 116, "Legenda:", 1, C.tekstDim)

  local SC = 22  -- mini vakgrootte (56 -> 22, factor ~0.39)
  local function mini(fn, mx, my, vul, rand)
    local s = SC / 56
    local function fr(x,y,w,h,c)         bkos.fillRect(math.floor(mx+x*s),math.floor(my+y*s),math.max(1,math.floor(w*s)),math.max(1,math.floor(h*s)),c) end
    local function dr(x,y,w,h,c)         bkos.drawRect(math.floor(mx+x*s),math.floor(my+y*s),math.max(1,math.floor(w*s)),math.max(1,math.floor(h*s)),c) end
    local function fc(x,y,r2,c)          bkos.fillCircle(math.floor(mx+x*s),math.floor(my+y*s),math.max(1,math.floor(r2*s)),c) end
    local function dc(x,y,r2,c)          bkos.drawCircle(math.floor(mx+x*s),math.floor(my+y*s),math.max(1,math.floor(r2*s)),c) end
    local function ft(x0,y0,x1,y1,x2,y2,c) bkos.fillTriangle(math.floor(mx+x0*s),math.floor(my+y0*s),math.floor(mx+x1*s),math.floor(my+y1*s),math.floor(mx+x2*s),math.floor(my+y2*s),c) end
    local function frr(x,y,w,h,r2,c)    bkos.fillRoundRect(math.floor(mx+x*s),math.floor(my+y*s),math.max(2,math.floor(w*s)),math.max(2,math.floor(h*s)),math.max(1,math.floor(r2*s)),c) end
    local function drr(x,y,w,h,r2,c)    bkos.drawRoundRect(math.floor(mx+x*s),math.floor(my+y*s),math.max(2,math.floor(w*s)),math.max(2,math.floor(h*s)),math.max(1,math.floor(r2*s)),c) end
    if fn == PION then
      frr(14,43,28,8,3,vul); drr(14,43,28,8,3,rand)
      fr(23,34,10,11,vul);   dr(23,34,10,11,rand)
      fc(28,26,11,vul);      dc(28,26,11,rand)
    elseif fn == TOREN then
      frr(10,44,36,8,2,vul); drr(10,44,36,8,2,rand)
      fr(15,22,26,24,vul);   dr(15,22,26,24,rand)
      fr(13,14,8,10,vul);    dr(13,14,8,10,rand)
      fr(24,14,8,10,vul);    dr(24,14,8,10,rand)
      fr(35,14,8,10,vul);    dr(35,14,8,10,rand)
    elseif fn == PAARD then
      frr(11,44,32,8,2,vul); drr(11,44,32,8,2,rand)
      ft(18,44,38,44,32,22,vul)
      fr(20,18,14,26,vul);   dr(20,18,14,26,rand)
      frr(22,12,22,14,4,vul); drr(22,12,22,14,4,rand)
      fc(38,15,2,rand)
      ft(24,12,28,12,26,7,vul)
    elseif fn == LOPER then
      frr(12,44,32,8,3,vul); drr(12,44,32,8,3,rand)
      frr(16,37,24,9,2,vul); drr(16,37,24,9,2,rand)
      ft(16,37,40,37,28,18,vul)
      fc(28,16,8,vul);       dc(28,16,8,rand)
      fc(28,9,3,vul);        dc(28,9,3,rand)
    elseif fn == KONINGIN then
      frr(9,44,38,8,3,vul);  drr(9,44,38,8,3,rand)
      ft(12,42,44,42,28,24,vul)
      frr(20,23,16,20,2,vul); drr(20,23,16,20,2,rand)
      fc(28,19,8,vul);        dc(28,19,8,rand)
      ft(19,14,23,14,21,8,vul); ft(26,13,30,13,28,6,vul); ft(33,14,37,14,35,8,vul)
      fc(21,8,2,vul); dc(21,8,2,rand)
      fc(28,6,2,vul); dc(28,6,2,rand)
      fc(35,8,2,vul); dc(35,8,2,rand)
    elseif fn == KONING then
      frr(9,44,38,8,3,vul);  drr(9,44,38,8,3,rand)
      ft(12,42,44,42,28,26,vul)
      frr(20,25,16,18,2,vul); drr(20,25,16,18,2,rand)
      fc(28,20,8,vul);        dc(28,20,8,rand)
      fr(26,6,4,14,vul);     dr(26,6,4,14,rand)
      fr(21,9,14,4,vul);     dr(21,9,14,4,rand)
    end
  end

  -- 6 icoontjes in 2 kolommen, rij-hoogte 26px → eindigt op y=116+3*26=194
  local stukTypes = {KONING, KONINGIN, TOREN, LOPER, PAARD, PION}
  local stukNamen = {"Koning","Koningin","Toren","Loper","Paard","Pion"}
  for i=1,6 do
    local col = (i-1) % 2
    local row = math.floor((i-1)/2)
    local ix  = px + col * 142
    local iy  = 126 + row * 26
    bkos.fillRect(ix, iy, SC, SC, C.bg)
    mini(stukTypes[i], ix, iy, C.wit, C.tekst)
    bkos.drawText(ix + SC + 3, iy + 7, stukNamen[i], 1, C.tekst)
  end

  -- Knoppen: beginnen op y=210, hoogte=36, onderkant=246/290/334 — ruim boven nav bar
  bkos.drawFastHLine(px, 206, 285, C.border)
  if matOfPat ~= "" then
    tekenKnop(px, 212, 285, 36, "Nieuw spel", true,  2)
    tekenKnop(px, 254, 285, 36, "Menu",       false, 2)
  else
    tekenKnop(px, 212, 285, 36, "Nieuw spel", false, 2)
    tekenKnop(px, 254, 285, 36, "Menu",       false, 2)
    tekenKnop(px, 296, 285, 36, "Geef op",    false, 2)
  end
end

-- =============================================================
--  MENU  (startscherm)
-- =============================================================

local function tekenMenu()
  bkos.fillScreen(C.bg)

  -- Titel
  bkos.drawText(200, 30, "SCHAAKSPEL", 4, C.highlight)
  bkos.drawText(245, 72, "BKOS Boordcomputer", 2, C.tekstDim)

  -- Bord decoratie (mini)
  local mx, my, ms = 290, 110, 28
  for r=0,7 do
    for k=0,7 do
      local kleur = (r+k)%2==0 and C.lichtVak or C.donkerVak
      bkos.fillRect(mx+k*ms, my+r*ms, ms, ms, kleur)
    end
  end
  bkos.drawRect(mx-1, my-1, 8*ms+2, 8*ms+2, C.border)

  -- Knoppen op y=300, hoogte=44 → onderkant y=344, ruim binnen 396px content
  tekenKnop(190, 300, 180, 44, "2 Spelers",   false, 2)
  tekenKnop(390, 300, 180, 44, "vs Computer", false, 2)
end

-- =============================================================
--  KLEURKEUZE SCHERM
-- =============================================================

local function tekenKleurKeuze()
  bkos.fillScreen(C.bg)
  bkos.drawText(220, 15, "Kies uw kleur", 3, C.tekst)

  -- Wit vlak: y=60..220
  bkos.fillRoundRect(130, 60, 180, 160, 12, C.lichtVak)
  bkos.drawText(175, 95, "K", 8, C.wit)
  bkos.drawText(182, 212, "Wit", 2, C.tekst)

  -- Zwart vlak: y=60..220
  bkos.fillRoundRect(470, 60, 180, 160, 12, C.donkerVak)
  bkos.drawText(515, 95, "K", 8, C.zwart)
  bkos.drawText(477, 212, "Zwart", 2, C.tekst)

  bkos.drawText(255, 242, "Wit begint altijd!", 1, C.tekstDim)
  -- Terug knop: y=265, onderkant=309 — ruim binnen 396px
  tekenKnop(300, 265, 200, 44, "Terug", false, 2)
end

-- =============================================================
--  NIVEAUKEUZE SCHERM
-- =============================================================

local function tekenNiveauKeuze()
  bkos.fillScreen(C.bg)
  local jouwKleur = spelersKleur==1 and "Wit" or "Zwart"
  bkos.drawText(160, 10, "Moeilijkheidsgraad", 3, C.tekst)
  bkos.drawText(290, 48, "U speelt: "..jouwKleur, 2, C.highlight)

  local niveaus = {"Makkelijk","Gemiddeld","Moeilijk"}
  local omschrijving = {
    "Computer speelt willekeurig",
    "Denkt 2 zetten vooruit",
    "Denkt 3 zetten vooruit",
  }
  -- Knoppen op y=75, 145, 215  hoogte=50, onderlinge ruimte=70
  for i, naam in ipairs(niveaus) do
    local actief = niveauComp == i
    tekenKnop(240, 75 + (i-1)*70, 320, 50, naam, actief, 2)
    bkos.drawText(270, 130 + (i-1)*70, omschrijving[i], 1, C.tekstDim)
  end

  -- Terug / Start knoppen op y=305, onderkant=349 — veilig binnen 396px
  tekenKnop(200, 305, 160, 44, "Terug",  false, 2)
  tekenKnop(390, 305, 160, 44, "Start!", true,  2)
end

-- =============================================================
--  PROMOTIE SCHERM
-- =============================================================

local function tekenPromoKeuze()
  bkos.fillRoundRect(200, 150, 400, 180, 12, C.panel)
  bkos.drawRoundRect(200, 150, 400, 180, 12, C.border)
  bkos.drawText(270, 162, "Kies promotie:", 2, C.tekst)

  local opties = {
    {KONINGIN, "Q", "Koningin"},
    {TOREN,    "T", "Toren"},
    {LOPER,    "L", "Loper"},
    {PAARD,    "P", "Paard"},
  }
  for i, opt in ipairs(opties) do
    local x = 210 + (i-1)*96
    bkos.fillRoundRect(x, 195, 86, 100, 8, C.knopInac)
    bkos.drawRoundRect(x, 195, 86, 100, 8, C.border)
    bkos.drawText(x+25, 205, opt[2], 5, C.highlight)
    bkos.drawText(x+4,  255, opt[3], 1, C.tekst)
  end
end

-- =============================================================
--  HOOFD TEKEN CALLBACK
-- =============================================================

bkos.draw = function()
  if staat == MENU then
    tekenMenu()
  elseif staat == KLEUR then
    tekenKleurKeuze()
  elseif staat == NIVEAU then
    tekenNiveauKeuze()
  elseif staat == SPEL then
    bkos.fillScreen(C.bg)
    tekenBord()
    tekenPanel()
    if staat == PROMO then
      tekenPromoKeuze()
    end
  elseif staat == PROMO then
    -- Teken bord + promotie overlay
    bkos.fillScreen(C.bg)
    tekenBord()
    tekenPanel()
    tekenPromoKeuze()
  end
end

-- =============================================================
--  TOUCH: BORD KLIK
-- =============================================================

local function bordKlik(r, k)
  if matOfPat ~= "" then return end

  -- Als het niet jouw beurt is (computer modus), negeer
  if tegenComputer and aanBeurt ~= spelersKleur then return end

  local stuk = bord[r][k]

  if geselecteerd then
    -- Kijk of (r,k) een mogelijke zet is
    local gevonden = nil
    for _, z in ipairs(mogelijkeZetten) do
      if z[1]==r and z[2]==k then
        gevonden = z; break
      end
    end

    if gevonden then
      -- Promotie check
      local selStuk = bord[geselecteerd[1]][geselecteerd[2]]
      local isPromotie = absStuk(selStuk)==PION and (r==8 or r==1)
      if isPromotie then
        promoZet = {geselecteerd[1], geselecteerd[2], r, k, gevonden[3]}
        staat = PROMO
      else
        pasZetToe(geselecteerd[1], geselecteerd[2], r, k, gevonden[3], nil)
        geselecteerd = nil
        mogelijkeZetten = {}
      end
    elseif kleurVan(stuk) == aanBeurt then
      -- Selecteer ander stuk van zelfde kleur
      geselecteerd = {r, k}
      mogelijkeZetten = legaleZetten(bord, r, k, aanBeurt, enPassantKolom, enPassantRij, rokadeRechten)
    else
      -- Deselecteer
      geselecteerd = nil
      mogelijkeZetten = {}
    end
  else
    -- Selecteer stuk
    if kleurVan(stuk) == aanBeurt then
      geselecteerd = {r, k}
      mogelijkeZetten = legaleZetten(bord, r, k, aanBeurt, enPassantKolom, enPassantRij, rokadeRechten)
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

  -- Computer speelt
  local z = computerZet()
  if z then
    local isPromotie = absStuk(bord[z[1]][z[2]])==PION and (z[3]==8 or z[3]==1)
    local promo = isPromotie and KONINGIN or nil  -- computer promoveert altijd naar koningin
    pasZetToe(z[1], z[2], z[3], z[4], z[5], promo)
    geselecteerd = nil
    mogelijkeZetten = {}
  end
end

-- =============================================================
--  TOUCH CALLBACK
-- =============================================================

bkos.touch = function(x, y)

  -- ── MENU ────────────────────────────────────────────────────
  if staat == MENU then
    -- "2 Spelers": x=190..370, y=300..344
    if x>=190 and x<=370 and y>=300 and y<=344 then
      tegenComputer = false
      initialiseerBord()
      staat = SPEL
    end
    -- "vs Computer": x=390..570, y=300..344
    if x>=390 and x<=570 and y>=300 and y<=344 then
      tegenComputer = true
      staat = KLEUR
    end

  -- ── KLEUR KEUZE ─────────────────────────────────────────────
  elseif staat == KLEUR then
    -- Wit: x=130..310, y=60..220
    if x>=130 and x<=310 and y>=60 and y<=220 then
      spelersKleur = 1
      staat = NIVEAU
    end
    -- Zwart: x=470..650, y=60..220
    if x>=470 and x<=650 and y>=60 and y<=220 then
      spelersKleur = -1
      staat = NIVEAU
    end
    -- Terug: x=300..500, y=265..309
    if x>=300 and x<=500 and y>=265 and y<=309 then
      staat = MENU
    end

  -- ── NIVEAU KEUZE ─────────────────────────────────────────────
  elseif staat == NIVEAU then
    -- Makkelijk: y=75..125
    if x>=240 and x<=560 and y>=75  and y<=125 then niveauComp=1 end
    -- Gemiddeld: y=145..195
    if x>=240 and x<=560 and y>=145 and y<=195 then niveauComp=2 end
    -- Moeilijk:  y=215..265
    if x>=240 and x<=560 and y>=215 and y<=265 then niveauComp=3 end
    -- Terug: x=200..360, y=305..349
    if x>=200 and x<=360 and y>=305 and y<=349 then staat=KLEUR end
    -- Start: x=390..550, y=305..349
    if x>=390 and x<=550 and y>=305 and y<=349 then
      initialiseerBord()
      staat = SPEL
    end

  -- ── PROMOTIE KEUZE ───────────────────────────────────────────
  elseif staat == PROMO then
    local opties = {KONINGIN, TOREN, LOPER, PAARD}
    for i, stuk in ipairs(opties) do
      local bx = 210 + (i-1)*96
      if x>=bx and x<=bx+86 and y>=195 and y<=295 then
        if promoZet then
          pasZetToe(promoZet[1], promoZet[2], promoZet[3], promoZet[4], promoZet[5], stuk)
          promoZet = nil
          geselecteerd = nil
          mogelijkeZetten = {}
          staat = SPEL
        end
        break
      end
    end

  -- ── SPEL ─────────────────────────────────────────────────────
  elseif staat == SPEL then
    local px = 492

    -- Knop: Nieuw spel (y=212..248)
    if x>=px and x<=px+285 and y>=212 and y<=248 then
      initialiseerBord()
      return
    end
    -- Knop: Menu (y=254..290)
    if x>=px and x<=px+285 and y>=254 and y<=290 then
      staat = MENU
      return
    end
    -- Knop: Geef op (y=296..332)
    if x>=px and x<=px+285 and y>=296 and y<=332 then
      berichtTekst = aanBeurt==1 and "Wit geeft op! Zwart wint." or "Zwart geeft op! Wit wint."
      matOfPat = "mat"
      return
    end

    -- Bord klik
    if x>=BX and x<BX+8*SQ and y>=BY and y<BY+8*SQ then
      local k = math.floor((x - BX) / SQ) + 1
      local r = 8 - math.floor((y - BY) / SQ)
      bordKlik(r, k)
    end
  end
end

-- =============================================================
--  INIT
-- =============================================================

-- Start in menu
staat = MENU

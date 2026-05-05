-- ─────────────────────────────────────────────────────────────────────────────
-- BKOS App: Tic-Tac-Toe
-- Two players, taking turns on the touchscreen.
-- Demonstrates: bkos.draw, bkos.touch, bkos.drawCircle, bkos.drawLine, bkos.fillRect
-- ─────────────────────────────────────────────────────────────────────────────

local EMPTY    = 0
local PLAYER_X = 1
local PLAYER_O = 2

local board          = {}
local currentPlayer  = PLAYER_X
local winner         = EMPTY
local gameOver       = false
local draw           = false

-- Layout (designed for 800×480; header/footer managed by BKOS-NUI)
local CEL    = 120
local GRID_X = (bkos.W - 3 * CEL) / 2   -- horizontally centered = 220
local GRID_Y = 48
local MARGIN = 16

-- Colors
local C_GRID  = bkos.color565(60,  85, 110)
local C_X     = bkos.colors.red
local C_O     = bkos.colors.cyan
local C_WIN   = bkos.colors.green
local C_BTN   = bkos.color565(40,  60,  80)
local C_BTN_T = bkos.colors.text

local function idx(r, c)  return r * 3 + c + 1  end

-- ─── Draw symbols ─────────────────────────────────────────────────────────────
local function drawX(cx, cy)
    local h = CEL / 2 - MARGIN
    for d = -1, 1 do
        bkos.drawLine(cx - h + d, cy - h, cx + h + d, cy + h, C_X)
        bkos.drawLine(cx + h + d, cy - h, cx - h + d, cy + h, C_X)
    end
end

local function drawO(cx, cy)
    local r = CEL / 2 - MARGIN
    bkos.drawCircle(cx, cy, r,     C_O)
    bkos.drawCircle(cx, cy, r - 1, C_O)
    bkos.drawCircle(cx, cy, r - 2, C_O)
end

-- ─── Win check ────────────────────────────────────────────────────────────────
local WIN_LINES = {
    {1,2,3}, {4,5,6}, {7,8,9},
    {1,4,7}, {2,5,8}, {3,6,9},
    {1,5,9}, {3,5,7}
}

local function checkWin()
    for _, line in ipairs(WIN_LINES) do
        local a, b, c = board[line[1]], board[line[2]], board[line[3]]
        if a ~= EMPTY and a == b and b == c then return a end
    end
    return EMPTY
end

local function isFull()
    for i = 1, 9 do
        if board[i] == EMPTY then return false end
    end
    return true
end

-- ─── New game ─────────────────────────────────────────────────────────────────
local function newGame()
    board = {}
    for i = 1, 9 do board[i] = EMPTY end
    currentPlayer = PLAYER_X
    winner        = EMPTY
    gameOver      = false
    draw          = false
end

newGame()

-- ─── Draw screen ──────────────────────────────────────────────────────────────
function bkos.draw()
    bkos.fillScreen(bkos.colors.bg)

    -- Status bar at top
    local status, sColor
    if gameOver then
        if winner ~= EMPTY then
            status = (winner == PLAYER_X) and "Player X wins!" or "Player O wins!"
            sColor = C_WIN
        else
            status = "Draw — PLAY AGAIN?"
            sColor = bkos.colors.amber
        end
    elseif currentPlayer == PLAYER_X then
        status = "Player X to move"
        sColor = C_X
    else
        status = "Player O to move"
        sColor = C_O
    end
    bkos.fillRect(0, 0, bkos.W, GRID_Y - 4, bkos.color565(18, 26, 36))
    local tx = math.floor((bkos.W - #status * 12) / 2)
    bkos.drawText(tx, 18, status, 2, sColor)

    -- Grid lines (4px wide)
    for i = 1, 2 do
        bkos.fillRect(GRID_X + i * CEL - 2, GRID_Y,     4, 3 * CEL, C_GRID)
        bkos.fillRect(GRID_X,               GRID_Y + i * CEL - 2, 3 * CEL, 4, C_GRID)
    end

    -- Cell contents
    for r = 0, 2 do
        for c = 0, 2 do
            local v  = board[idx(r, c)]
            local cx = GRID_X + c * CEL + math.floor(CEL / 2)
            local cy = GRID_Y + r * CEL + math.floor(CEL / 2)
            if v == PLAYER_X then drawX(cx, cy)
            elseif v == PLAYER_O then drawO(cx, cy)
            end
        end
    end

    -- PLAY AGAIN button below grid
    local kx = math.floor(bkos.W / 2) - 90
    local ky = GRID_Y + 3 * CEL + 18
    bkos.fillRect(kx, ky, 180, 48, C_BTN)
    bkos.drawText(kx + 16, ky + 16, "PLAY AGAIN", 2, C_BTN_T)
end

-- ─── Touch handling ───────────────────────────────────────────────────────────
function bkos.touch(x, y)
    local kx = math.floor(bkos.W / 2) - 90
    local ky = GRID_Y + 3 * CEL + 18
    if x >= kx and x <= kx + 180 and y >= ky and y <= ky + 48 then
        newGame()
        bkos.draw()
        return
    end

    if gameOver then return end

    if x >= GRID_X and x < GRID_X + 3 * CEL and
       y >= GRID_Y and y < GRID_Y + 3 * CEL then
        local c = math.floor((x - GRID_X) / CEL)
        local r = math.floor((y - GRID_Y) / CEL)
        local i = idx(r, c)
        if board[i] == EMPTY then
            board[i] = currentPlayer
            winner   = checkWin()
            if winner ~= EMPTY then
                gameOver = true
            elseif isFull() then
                draw     = true
                gameOver = true
            else
                currentPlayer = (currentPlayer == PLAYER_X) and PLAYER_O or PLAYER_X
            end
            bkos.draw()
        end
    end
end

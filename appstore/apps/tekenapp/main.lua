-- ============================================================
--  TEKEN APP  v1.0  –  BKOS Lua App API v2
--  Heel scherm zwart, touch = witte stip
-- ============================================================

bkos.draw = function()
  bkos.fillScreen(bkos.color565(0, 0, 0))
end

bkos.touch = function(x, y)
  bkos.fillCircle(x, y, 6, bkos.color565(255, 255, 255))
end

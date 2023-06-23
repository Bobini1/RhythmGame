local function onUpdate(self, delta)
    count = count + 1
    time = time + delta
    if time > 1 then
        self.text = "FPS: " .. tostring(count)
        count = 0
        time = 0
        local function currentSizeUpdater(value)
            self.fillColor = { r = 255, g = 255, b = 255, a = math.floor(value) }
        end
        local animation = Linear.new(currentSizeUpdater, 1, 0, 255)
        playAnimation(animation)
    end
end
local fps = Text.new { text = "FPS: 0", fillColor = { r = 255, g = 0, b = 0, a = 255 }, onUpdate = onUpdate, isWidthManaged = true }
local function createNote(offset_from_top)
    return Padding.new {
        child = Quad.new {
            width = 100,
            height = 20,
            fillColor = { r = 255, g = 0, b = 0, a = 255 },
            isWidthManaged = true,
            isHeightManaged = true
        },
        top = offset_from_top
    }
end

local bg = Quad.new { isWidthManaged = true, isHeightManaged = true, fillColor = { r = 255, g = 0, b = 255, a = 255 } }
local game_bg = Quad.new { width = 700, isHeightManaged = true, fillColor = { r = 255, g = 0, b = 255, a = 255 } }
local columns = {}
local column_width = game_bg.width / 7
for i = 1, 7 do
    columns[i] = Quad.new { width = column_width, isHeightManaged = true, fillColor = { r = 255, g = 0, b = 255, a = 255 } }
end

return layers
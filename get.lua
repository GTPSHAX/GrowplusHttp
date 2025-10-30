-- get.lua
local max_requests = 400

-- Setiap koneksi punya counter sendiri
function init(args)
    counter = 0
end

request = function()
    if counter >= max_requests then
        return nil  -- stop request setelah 400
    end
    counter = counter + 1
    -- Ganti path sesuai file lo, misal file1.ext, file2.ext, dst.
    local path = "/"
    return wrk.format("GET", path)
end

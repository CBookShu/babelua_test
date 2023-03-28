print("hello world")


local count = 100
local i = 0
test_func = function()
    i = i + 1
    print("timer ", i)
end

set_timer("test_func", 2, count)
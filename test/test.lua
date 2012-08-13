require("elprofiler")
elprofiler.start()
--kprofile.open('file://profile.out')
--kprofile.start()

function func1()
--	print("hello")
	os.time()
end

function func2()
	for i=1, 1000000 do
		func1()
	end
--	print("empty")
	os.time()
	coroutine.yield()
end

co = coroutine.create(func2)

print("yes")

local code = [[return function ()
--	print('ret')
	os.time()
	end]]
f = loadstring(code)

f()()
print("mark1")
coroutine.resume(co)
print("mark2")
elprofiler.stop()
print("end")

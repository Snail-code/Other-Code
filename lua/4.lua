function newCounter()
	local i =0;
	print("local " .. i)
	return function()
		print("return" .. i)
		i = i +1;
		return i
	end
end

c1 = newCounter()  
--//这里执行了函数
print("-------------------------")
print(c1())
print("-------------------------")
print(c1())

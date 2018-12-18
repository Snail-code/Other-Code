require ("md5")

local  lctoken =  "c7ec63535323af24"
local pk = "9f871a360cb59f6d"
local  ouid = "365038a116b17451e2281583070afa63"

local time2 = os.time() + 18000  

local  lctoken = lctoken .. time2.. ouid .. pk
print("lctoken = ",lctoken)
local lctoken_md5 = md5.sumhexa(lctoken)


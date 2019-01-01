package main

import (
	_ "quickstart/routers"

	"github.com/astaxie/beego"

	_ "quickstart/models"
)

func main() {
	beego.Run()
}

// beego.AppConfig.String("mysqlpass")
// beego.AppConfig.String("mysqlurls")
// beego.AppConfig.String("mysqldb")

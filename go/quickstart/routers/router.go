package routers

import (
	"quickstart/controllers"

	"github.com/astaxie/beego"
)

func init() {
	beego.Router("/get", &controllers.MainController{})
	beego.Router("/post", &controllers.PostController{})
	beego.Router("/ajax", &controllers.AjaxController{})
}

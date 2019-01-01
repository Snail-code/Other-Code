package controllers

import (
	_ "quickstart/import"

	"github.com/astaxie/beego"
)

type MainController struct {
	beego.Controller
}

func (c *MainController) Get() {
	c.Data["Website"] = "beego.me"
	c.Data["Email"] = "astaxie@gmail.com"
	c.TplName = "index.tpl"
}
func (c *MainController) Post() {
	firstname := c.Input().Get("firstname")
	lastname := c.Input().Get("lastname")
	username := c.Input().Get("username")
	// email := c.Input().Get("email")
	// password := c.Input().Get("password")
	beego.Debug(firstname)
	beego.Debug(lastname)
	beego.Debug(username)
}

type PostController struct {
	beego.Controller
}

func (c *PostController) Get() {
	c.Data["Website"] = "beego.me"
	c.Data["Email"] = "astaxie@gmail.com"
	c.TplName = "index.tpl"
}
func (c *PostController) Post() {
	firstname := c.Input().Get("firstname")
	lastname := c.Input().Get("lastname")
	username := c.Input().Get("username")
	// email := c.Input().Get("email")
	// password := c.Input().Get("password")
	beego.Debug(firstname)
	beego.Debug(lastname)
	beego.Debug(username)
	resp := make(map[string]interface{})
	resp["res"] = 1
	resp["status"] = 200
	c.Data["json"] = resp
	defer c.ServeJSON()
	// return
	// c.TplName = "post.tpl"
}

type AjaxController struct {
	beego.Controller
}

func (c *AjaxController) Get() {
	c.Data["Website"] = "beego.me"
	c.Data["Email"] = "astaxie@gmail.com"
	c.TplName = "index.tpl"
}
func (c *AjaxController) Post() {
	beego.Debug("ajax")
	username := c.GetString("username")
	password := c.GetString("password")
	beego.Debug(username)
	beego.Debug(password)
	resp := make(map[string]interface{})
	resp["res"] = 1
	resp["status"] = 200
	c.Data["json"] = resp

	defer c.ServeJSON()
}

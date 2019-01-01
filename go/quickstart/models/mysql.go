package MysqlDB

import (
	"fmt"
	"strconv"

	"github.com/astaxie/beego"
	"github.com/astaxie/beego/orm"
	_ "github.com/go-sql-driver/mysql"
)

/**
*数据相关配置
 */
type DBConfig struct {
	Host         string
	Port         string
	Database     string
	Username     string
	Password     string
	MaxIdleConns int //最大空闲连接
	MaxOpenConns int //最大连接数
}

func init() {
	orm.RegisterDriver("mysql", orm.DRMySQL)
	var dbconfig DBConfig
	dbconfig.Database = beego.AppConfig.String("mysqldb")
	dbconfig.Host = beego.AppConfig.String("mysqlhost")
	dbconfig.MaxIdleConns, _ = strconv.Atoi(beego.AppConfig.String("sqlMaxIdleConns"))
	dbconfig.MaxOpenConns, _ = strconv.Atoi(beego.AppConfig.String("sqlMaxOpenConns"))
	dbconfig.Password = beego.AppConfig.String("mysqlpassword")
	dbconfig.Port = beego.AppConfig.String("mysqlport")
	dbconfig.Username = beego.AppConfig.String("mysqluser")
	ds := fmt.Sprintf("%s:%s@tcp(%s:%s)/%s?charset=utf8", dbconfig.Username, dbconfig.Password, dbconfig.Host, dbconfig.Port, dbconfig.Database)
	beego.Debug(ds)
	err := orm.RegisterDataBase("default", "mysql", ds, dbconfig.MaxIdleConns, dbconfig.MaxOpenConns)
	if err != nil {
		panic(err)
	}
}

func Login(UserName string, Password string) {
	var maps []orm.Params
	o := orm.NewOrm()
	num, err := o.Raw("select password from tb_user where user_name = ;", UserName).Values(&maps)
	if err != nil {
		panic(err)
	}
	beego.Debug(num)

	if err == nil && num > 0 {
		// beego.Debug(maps[0]["id"])
		// beego.Debug(maps[0]["user_name"])
		// beego.Debug(maps[0]["password"])
		// beego.Debug(maps[0]["nickname"])
		// beego.Debug(maps[1]["id"])
		// beego.Debug(maps[1]["user_name"])
		// beego.Debug(maps[1]["password"])
		// beego.Debug(maps[1]["nickname"])
	}
}

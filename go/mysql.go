package main

import (
	"database/sql"
	"fmt"

	_ "github.com/go-sql-driver/mysql"
)

func checkErr(err error) {

	if err != nil {

		panic(err)

	}

}

func main() {

	//打开数据库mytest

	fmt.Println("open the database, mytest")

	db, err := sql.Open("mysql", "root:Zxcvbnm,./1@@tcp(192.168.2.116)/test?charset=utf8&allowOldPasswords=1")
	if err != nil {
		checkErr(err)
		return
	}
	defer db.Close()
	fmt.Println("open success")
	// insertToDB(db)
	QueryFromDB(db)
	UpdateDB(db, 3)
	QueryFromDB(db)
	DeleteFromDB(db, 1)
	QueryFromDB(db)
}

func insertToDB(db *sql.DB) {
	stmt, err := db.Prepare("insert user_info set id=?,name=?")
	checkErr(err)
	res, err := stmt.Exec(3, "peter")
	checkErr(err)
	id, err := res.LastInsertId()
	checkErr(err)
	if err != nil {
		fmt.Println("插入数据失败")
	} else {
		fmt.Println("插入数据成功：", id)
	}

}

func QueryFromDB(db *sql.DB) {
	rows, err := db.Query("SELECT * FROM user_info")
	checkErr(err)
	if err != nil {
		fmt.Println("error:", err)
	} else {
	}
	for rows.Next() {
		var id string
		var name string
		checkErr(err)
		err = rows.Scan(&id, &name)
		fmt.Println(id)
		fmt.Println(name)
	}
}

func UpdateDB(db *sql.DB, uid int) {
	stmt, err := db.Prepare("update user_info set name=? where id=?")
	checkErr(err)
	res, err := stmt.Exec("zhangqi", uid)
	affect, err := res.RowsAffected()
	fmt.Println("更新数据：", affect)
	checkErr(err)
}

func DeleteFromDB(db *sql.DB, autid int) {
	stmt, err := db.Prepare("delete from user_info where id=?")
	checkErr(err)
	res, err := stmt.Exec(autid)
	affect, err := res.RowsAffected()
	fmt.Println("删除数据：", affect)
}

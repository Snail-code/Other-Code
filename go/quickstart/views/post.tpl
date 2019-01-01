<!DOCTYPE html>
<html lang="zh-cn">
<head>
<meta charset="utf-8">
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>login</title>

<!-- <link href="css/bootstrap.min.css" rel="stylesheet"> --> <!-- Bootstrap -->
<link href="https://cdn.jsdelivr.net/npm/bootstrap@3.3.7/dist/css/bootstrap.min.css" rel="stylesheet">

</head>

<body>

<!-- 详细信息模态框（Modal） -->
            <div>
                <div class="modal fade" id="queryInfo" tabindex="-1" role="dialog"
                    aria-labelledby="myModalLabel" aria-hidden="true">
                    <div class="modal-dialog">
                        <div class="modal-content">
                            <div class="modal-header">
                                <button type="button" class="close" data-dismiss="modal"
                                    aria-hidden="true">&times;</button>
                                <h4 class="modal-title" id="myModalLabel">详细信息</h4>
                            </div>
                            <form
                                action="${pageContext.request.contextPath }/productServlet?type=info"
                                method="post">
                                <div class="modal-body">
                                    <div class="input-group">
                                        <span class="input-group-addon">名&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;称</span>
                                        <input type="text" class="form-control" placeholder="请输入名称"
                                            id="name1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">规格及型号</span> <input
                                            type="text" class="form-control" placeholder="请输入规格及型号"
                                             id="xinghao1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">存&nbsp;放&nbsp;地&nbsp;点</span>
                                        <input type="text" class="form-control"
                                            placeholder="请输入存放地点名称" id="address1"
                                            readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">使&nbsp;用&nbsp;部&nbsp;门</span>
                                        <input type="text" class="form-control"
                                            placeholder="请输入使用部门名称" id="department1"
                                            readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">单&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;位</span>
                                        <input type="text" class="form-control" placeholder="请输入单位名称"
                                            id="unit1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">数&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;量</span>
                                        <input type="text" class="form-control" placeholder="请输入数量"
                                            id="number1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">单&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;价</span>
                                        <input type="text" class="form-control" placeholder="请输入单价"
                                            id="price1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">金&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;额</span>
                                        <input type="text" class="form-control" placeholder="请输入金额"
                                            id="totalprice1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">来&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;源</span>
                                        <input type="text" class="form-control" placeholder="请输入来源"
                                            id="come1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">购&nbsp;建&nbsp;日&nbsp;期</span>
                                        <input type="text" class="form-control" placeholder="请输入购建日期"
                                            id="buytime1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">使&nbsp;&nbsp;&nbsp;用&nbsp;&nbsp;&nbsp;人</span>
                                        <input type="text" class="form-control" placeholder="请输入使用人名称"
                                            id="useperson1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">经&nbsp;&nbsp;&nbsp;办&nbsp;&nbsp;&nbsp;人</span>
                                        <input type="text" class="form-control" placeholder="请输入经办人名称"
                                            id="handleperson1" readonly="readonly">
                                    </div>
                                    <br />
                                    <div class="input-group">
                                        <span class="input-group-addon">管&nbsp;&nbsp;&nbsp;理&nbsp;&nbsp;&nbsp;员</span>
                                        <input type="text" class="form-control" placeholder="请输入管理员名称"
                                            id="admini1" readonly="readonly">
                                    </div>
                                </div>
                                <div class="modal-footer">
                                    <button type="button" class="btn btn-default"
                                        data-dismiss="modal">关闭</button>
                                    <!-- <button type="submit" class="btn btn-primary">提交</button> -->
                                </div>
                            </form>
                        </div>
                        <!-- /.modal-content -->
                    </div>
                    <!-- /.modal -->
                </div>


    <!-- jQuery (Bootstrap 的所有 JavaScript 插件都依赖 jQuery，所以必须放在前边) -->
    <script src="https://cdn.jsdelivr.net/npm/jquery@1.12.4/dist/jquery.min.js"></script>
    <!-- 加载 Bootstrap 的所有 JavaScript 插件。你也可以根据需要只加载单个插件。 -->
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@3.3.7/dist/js/bootstrap.min.js"></script>
</body>
</html>

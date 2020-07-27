var express = require("express");
var app = express();
app.use(express.static('public'));
app.use("/assets", express.static(__dirname + "/assets"));
app.use("/image", express.static(__dirname + "/image"));
//app.use(express.static(path.join(__dirname, 'public')));
app.set("view engine", "ejs");
app.set("views", "./views");
var server = require("http").Server(app);
var io = require("socket.io")(server);
var mysql = require("mysql");
var cookieParser = require('cookie-parser');
app.use(cookieParser());
server.listen(process.env.PORT || 3000);
var UserGlobal = new Map()
var current_User
var count_Package = 0
var Save_Number_Of_New_Relay
var Save_Lock_Status

// add get datetime S
//var today = new Date();
var dateTime;
var timelog;
var LogID;
var DateU;
var idKey = "02061996";
LogID = timelog;
DateU = dateTime;
// add get datetime E

// var db = mysql.createConnection({
//     host: "localhost",
//     port: "3306",
//     user: "root",
//     password: "123456",
//     database: "demo"
// });

var db = mysql.createConnection({
    host: "us-cdbr-east-05.cleardb.net",
    user: "bd2a709b83a1f6",
    port: "3306",
    password: "d94e858a",
    database: "heroku_9903738a6a1e2b4"
});

db.connect(function (err) {
    if (err) throw err;
    console.log("mysql Connected!");
    Create_New_table();
    Create_New_Log_table();
    Create_New_Key_table();
    updateTime();
});

db.on('error', function (err) {
    console.log("[mysql error]", err);
});

// function Create_New_table() {
//     var sql = "CREATE TABLE IF NOT EXISTS client (id INT PRIMARY KEY AUTO_INCREMENT,\
//                 Username VARCHAR(255), Password VARCHAR(255), LockState INT default 0 ,\
//                  Relay_1 INT default 0, Relay_2 INT default 0, Remote INT default 0, \
//                  End_Node INT default 0,Provisioner INT default 0)";
//     db.query(sql, function (err, result) {
//         if (err) throw err;
//     });
// }

function Create_New_table() {
    var sql = "CREATE TABLE IF NOT EXISTS client (id INT PRIMARY KEY AUTO_INCREMENT,\
                Username VARCHAR(255), Password VARCHAR(255))";
    db.query(sql, function (err, result) {
        if (err) throw err;
    });
}

function Create_New_Log_table() {
    var sql = "CREATE TABLE IF NOT EXISTS LogTable (idLog INT PRIMARY KEY AUTO_INCREMENT,\
                Time VARCHAR(255), DateTime VARCHAR(255), LockState INT default 0 ,\
                 Relay_1 INT default 0, Relay_2 INT default 0, Remote INT default 0, \
                 End_Node INT default 0,Provisioner INT default 0)";
    db.query(sql, function (err, result) {
        if (err) throw err;
    });

    db.query("INSERT INTO LogTable(DateTime) VALUES(?)", ["initial"], function (err, rows, fields) {
        if (err) throw err;
    })
}

function Create_New_Key_table() {
    var sql = "CREATE TABLE IF NOT EXISTS KeyTable (KeyID VARCHAR(255) PRIMARY KEY,\
                StartDate VARCHAR(255), EndDate VARCHAR(255))";
    db.query(sql, function (err, result) {
        if (err) throw err;
    });

    // db.query("INSERT INTO KeyTable(KeyID) VALUES(?)", [idKey], function (err, rows, fields) {
    //     if (err) throw err;
    // })
}

function updateTime() {
    // var ts = Date.now();
    // var date_ob = new Date(ts);
    // var date = date_ob.getDate();
    // var month = date_ob.getMonth() + 1;
    // var year = date_ob.getFullYear();
    var today = new Date();
    var asiaTime = new Date().toLocaleString("en-US", { timeZone: "Asia/Ho_Chi_Minh" });
    // timelog = today.getHours()+ "h" +today.getMinutes()+ "m" + today.getSeconds();
    // dateTime = today.getDate()+ "d" +(today.getMonth()+1)+ "m" + today.getFullYear();
    timelog = today.getDate() + "d" + (today.getMonth() + 1) + "m" + today.getFullYear();
    dateTime = new Date(asiaTime).toISOString();
    // timelog = date;
    // dateTime = date + "/" month + year;
}

io.on("connection", function (socket) {

    console.log("Co nguoi ket noi:" + socket.id);

    socket.on("doorstateChanged", function (state) {
        count_Package++
        console.log("Server Sended:" + state + "Package Number: " + count_Package);
        io.emit("updateState", state);
    });

    socket.on("MeshNetWorkStatusRequest", function () {
        console.log("Server request update mesh network status")
        io.emit("UpdateMeshNetWorkStatus")
    })

    socket.on("FirstLoadHomePage", function () {
        console.log("Fisrt Load HomePage" + Save_Number_Of_New_Relay)
        io.emit("LockStateChange", Save_Lock_Status)
        io.emit("HomePageOK")
        io.emit("Homepage_AddNew_Button_", Save_Number_Of_New_Relay)
    })

    socket.on("AutoSyncMeshStatus", function () {
        socket.emit("InitUpdateMeshNetWorkSTT");
        //socket.emit("UpdateMeshNetWorkStatus");
        for (let i = 1; i < 5; i++) {
            setTimeout(function timer() {
                socket.emit("UpdateMeshNetWorkStatus");
            }, i * 800);
        }
    })

    socket.on("UpdateMeshSTTComplete", function () {
        console.log("Server status update OKOKOK")
        io.emit("UpdateMeshNetWorkSTT_OK")
    })

    socket.on("from", function (msg) {
        var temp = CheckSession(msg.cookie)
        console.log("temp: " + temp)
        if (temp != null)
            if (msg.type != temp)
                socket.emit("Change_Page", temp)

        console.log("cookie: " + msg.cookie)
        socket.player = true
        CompairSession(msg.cookie, socket)
        switch (msg.type) {
            case "trangchu":
                HandlerHomePage(socket, msg)
                break;
            case "login":
                HandlerLoginPage(socket, msg)
                break;
            case "register":
                console.log("Register")
                HandlerRegisterPage(socket, msg)
                break;
            default:
                break;
        }
        console.log("socket.name : " + socket.username)
    });

    socket.on("Logout", function () {
        UserGlobal.delete(socket.username)
        socket.emit("Back_To_Login")
    })

    socket.on("Homepage_AddNew_Button", function (data) {
        console.log("Gateway request add new button")
        Save_Number_Of_New_Relay = data
        socket.broadcast.emit("Homepage_AddNew_Button_", data)
    })

    socket.on("Update_Door_State", function (data) {
        //console.log("Update new State of Mesh Network: " + current_User)
        // log update
        setInterval(updateTime, 1000);
        // log update
        var lock_state = 0;

        if (data.LockState == 1) {
            lock_state = 1
        }
        Save_Lock_Status = lock_state
        console.log("Update new Door State" + data.LockState + " " + lock_state + "time: " + dateTime)
        //socket.broadcast.emit("LockStateChange", (lock_state, DateU));
        // db.query("UPDATE client SET  LockState = ? WHERE Username=? ", [lock_state, current_User], function (err, rows, fields) {
        //     if (err) throw err;
        //     console.log("Update new State of node OK")
        // })

        //log update
        // socket.emit("LockStateChangeTime", DateU)
        socket.broadcast.emit("LockStateChange", { lock_state, DateU });

        db.query("INSERT INTO LogTable(DateTime) VALUES(?)", [DateU], function (err, rows, fields) {
            if (err) throw err;
            socket.emit("Created new timeline")
        })

        db.query("UPDATE LogTable SET Time =?, LockState =? WHERE DateTime=? ", [LogID, lock_state, DateU], function (err, rows, fields) {
            if (err) throw err;
            console.log("Update new State of node OK")
        })
        //log update
    })

    socket.on("Mesh_Network_State", function (data) {
        var relay1_state = 0
        var relay2_state = 0
        var relay3_state = 0
        var relay4_state = 0
        var relay5_state = 0
        var relay6_state = 0
        var remote_state = 0
        var endnode = 0

        if (data.Relay1 == 1) {
            relay1_state = 1
        }
        if (data.Relay2 == 1) {
            relay2_state = 1
        }
        // if (data.Remote == 1) {
        //     remote_state = 1
        // }
        if (data.Remote >= 2) {
            remote_state = 1
        }
        if (data.EndNode == 1) {
            endnode = 1
        }
        if (data.Relay3 == 1) {
            relay3_state = 1
        }
        if (data.Relay4 == 1) {
            relay4_state = 1
        }
        if (data.Relay5 == 1) {
            relay5_state = 1
        }
        if (data.Relay6 == 1) {
            relay6_state = 1
        }
        socket.broadcast.emit("Update_Mesh_Node_Status", {
            "Remote": remote_state, "EndNode": endnode, "Relay2": relay2_state,
            "Relay1": relay1_state, "Relay3": relay3_state, "Relay4": relay4_state, "Relay5": relay5_state, "Relay6": relay6_state
        })
        socket.broadcast.emit("ProvisionerAlive", 1)
        // db.query("UPDATE client SET  Relay_1 = ? , Relay_2 = ? , Remote = ?, End_Node = ?,Provisioner = ?\
        //   WHERE Username=? ", [relay1_state, relay2_state, remote_state, endnode, 1, current_User], function (err, rows, fields) {
        //     if (err) throw err;
        //     console.log("Update new State of node OK")
        // })

        // update status of mesh to log table
        db.query("UPDATE LogTable SET  Relay_1 = ? , Relay_2 = ? , Remote = ?, End_Node = ?,Provisioner = ?\
        WHERE DateTime=? ", [relay1_state, relay2_state, remote_state, endnode, 1, DateU], function (err, rows, fields) {
            if (err) throw err;
            console.log("Update new State of node OK")
        })
        // update status of mesh to log table
    })

    socket.on("Process_End_Date", function (data) {
        var EndDate = data;
        db.query("UPDATE KeyTable SET EndDate = ? WHERE KeyID=? ", [EndDate, idKey], function (err, rows, fields) {
            if (err) throw err;
            console.log("Update new End date of key")
        })
    })

    socket.on("disconnect", function () {
        console.log(socket.id + " ngat ket noi!!!!!");
    });
});


function HandlerLoginPage(socket, msg) {
    if (socket.username != null) {
        var saveCookie = {
            "resUserName": socket.username,
            "resSessionkey": UserGlobal.get(socket.username)
        };
        socket.emit("Server_Login_Sucess", saveCookie);
        return
    }

    socket.on("Client_Login", function (data) {
        console.log("Login" + " " + data.user + " " + data.pass)
        var user_login = data.user.toString()
        var pass_login = data.pass.toString()
        Login(user_login, pass_login, socket)
    });
}

function Login(username, pass, socket) {
    console.log("TQT Test Login: " + UserGlobal.get(username))
    if (UserGlobal.get(username) == undefined) {
        db.query("SELECT * FROM client WHERE Username=? AND Password=?", [username, pass], function (err, rows, fields) {
            console.log(rows);
            if (err) throw err;
            if (rows.length == 0) {
                LoginFail(username, socket)
            } else {
                console.log("Login successfull")
                LoginSuccess(username, pass, socket)
            }
        })
    }
    else {
        console.log("user allready login")
        LoginFail(username, socket)
    }
}

function LoginSuccess(username, pass, socket) {
    console.log(username + " login success")
    var SessionKey = CreateSessionKey()
    var saveCookie = {
        "resUserName": username,
        "resSessionkey": SessionKey
    };
    console.log("Cookie Created:  " + SessionKey);
    UserGlobal.set(username, {
        "session": SessionKey,
        "page": "trangchu",
        "pwd": pass,
        "currentUser": username,
    })
    current_User = username.toString()
    socket.emit("Server_Login_Sucess", saveCookie);

}

function LoginFail(username, socket) {
    console.log(username + " login fail")
    socket.emit("Server_Login_Fail");
}

function CompairSession(session, socket) {
    socket.username = null
    for (const [k, v] of UserGlobal.entries()) {
        if (v.session.localeCompare(session) == 0) {
            socket.username = k
            break
        }
    }
    console.log(UserGlobal)
}

function CreateSessionKey() {
    var str = "";
    for (; str.length < 32; str += Math.random().toString(36).substr(2));
    var SessionKey = str.substr(0, 32);
    return SessionKey
}

function CheckSession(session) {
    for (const v of UserGlobal.values()) {
        if (v.session.localeCompare(session) == 0) {
            console.log("change page")
            return v.page
        }
    }
    return null
}

function randomNumber() {
    var number = Math.random()
    console.log("random number : " + number)
    return (number > 0.5) ? 1 : 0;
}

function RegisterSuccess(username, password, socket) {
    db.query("INSERT INTO client(Username,Password) VALUES(?, ?)", [username, password], function (err, rows, fields) {
        if (err) throw err;
        console.log("add user " + username)
        console.log("register suscess");
        socket.emit("Sign_Up_Successfully")
    })
}

function RegisterFail(username, socket) {
    console.log("user allready exist")
    console.log("registr fail");
    socket.emit("Sign_Up_fail");
}

function HandlerHomePage(socket, msg) {
    if (socket.username == null) {
        console.log("undefined player at index, kick it out")
        socket.emit("Cookie_Fail");
    } else
        console.log(socket.username + " player at index")
}

function HandlerRegisterPage(socket, msg) {
    socket.on("Sign_Up", function (data) {
        var user = data.username.toString()
        var pass = data.password.toString()

        Register(user, pass, socket)
    });
}

function Register(username, pass, socket) {
    db.query("SELECT * FROM client WHERE Username=?", [username], function (err, rows, fields) {
        console.log(rows)
        if (err) throw err;
        if (rows.length == 0) {
            RegisterSuccess(username, pass, socket)
        } else {
            RegisterFail(username, socket)
        }
    })
}

app.get("/", function (req, res) {
    var temp = CheckSession(req.cookies.seasion)
    if (temp != null) {
        res.render(temp)
    } else
        res.render("login");
});

app.get("/login", function (req, res) {
    var temp = CheckSession(req.cookies.seasion)
    if (temp != null) {
        res.redirect(temp)
    } else
        res.render("login");
});

app.get("/trangchu", function (req, res) {
    var temp = CheckSession(req.cookies.seasion)
    if (temp == "trangchu") {
        res.render("trangchu")
    } else if (temp != null) {
        res.redirect(temp)
        return
    } else {
        res.redirect("login");
    }
});

app.get("/register", function (req, res) {
    res.render("register");
});

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mongoose.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sqlite3.h>
#include <libxml/parser.h>
#include <ctype.h>


#define maxLengthIdWorker  3 //LengthNumber(numberWorkers) 	// it has to be equal to the length of the maximum identifier used in a worker, the identifieres go from 0 to max number of workers. 
#define maxLengthIpNames 10 	//it establish tha maximum number of characters that would have any ip in the sistem
#define maxLengthPortNames 10	//it establish tha maximum number of characters that would have any name of a port in the sistem
#define maxRequestLength 30

//---database-----
sqlite3 *db;
char *zErrMsg = 0;
int rc;

// -----
struct file_data {
  FILE *fp;
  const char *data;
  int data_len;
  int written;
};

static const char *html_problem_add =
"<html>"
"<style type=\"text/css\">"
"body{ text-align: center; font-family: Tahoma, Arial, Geneva, sans-serif; color:#797979;}"
"#cont{  width: 1000px; margin: 0 auto 0 auto; text-align: left;}"

"#title{ height: 200px; margin-bottom: 50px; width: 750px; background: url(Images/title2.jpg) no-repeat; overflow: hidden; text-indent: -9999%; display: block;} "

"#codeTitle {position: relative; top: 30px;text-align: center; font-family: Tahoma; color:#898989; text-align: left;}"


"#code { width: 500px; margin-bottom: 70px; padding: 58px 75px 58px 69px;}"
"#code h2{ font-size: 30px; color: #464545; font-weight: normal }"
"#code {font-size: 20px;}       "
 
"#menu{ width: 180px; float: right; text-transform: uppercase; margin-bottom: 50px;}    "
"#menu ul li{border-left: 4px solid #e93700; margin-top: 6px; padding: 2px 0 2px 4px; list-style:none; }"
"#menu ul li a{ font-size: 12px; color: #464545; text-decoration: none;}"
"#menu ul li a:hover{ color: #e93700 }"
"#menu ul{ margin:0; padding-left:80px; }"
"#menu h3 {color: #e93700; font-size: 18px; font-weight: normal; padding-left: 80px;}"

"#pie{ clear: both;  height: 296px; background: url(Images/logo_universite_bobo.jpg) no-repeat; background-position: bottom right;}"
"#boxCenterside{background: url(Images/logo_universite_bobo.jpg) repeat-x scroll center top transparent; min-height: 100px; width: 260px;}"


"</style>"

"<head>"
"<title>Juge</title>"
"</head>"
 
"<body>"
 
"<div id=\"cont\">"
	
"<div id=\"title\">"
"</div>"
"<div id=\"codeTitle\">"
"<h2>TRYING TO ADD YOUR PROBLEM, testing if it is correct</h2>"	
"<br>"
"</div>";

static const char *html_waiting_form1 =
  "<html>"
"<style type=\"text/css\">"
"body{ text-align: center; font-family: Tahoma, Arial, Geneva, sans-serif; color:#797979;}"
"#cont{  width: 1000px; margin: 0 auto 0 auto; text-align: left;}"

"#title{ height: 200px; margin-bottom: 50px; width: 750px; background: url(Images/title2.jpg) no-repeat; overflow: hidden; text-indent: -9999%; display: block;} "

"#codeTitle {position: relative; top: 30px;text-align: center; font-family: Tahoma; color:#898989; text-align: left;}"


"#cajaarriba{"
"	height: 5px;"
"	width: 2000px;"
"	background-image: url(Images/arriba.gif);"
"	background-repeat: no-repeat;"
"	padding: 58px 75px 58px 69px;"
"}"

"#cajaabajo {"
"	height: 5px;"
"	width: 2000px;"
"	background-image: url(Images/abajo.gif);"
"	background-repeat: no-repeat;"
"	padding: 58px 75px 58px 69px;"
"} "
"#code { width: 500px; margin-bottom: 70px; padding: 58px 75px 58px 69px;}"
"#code h2{ font-size: 30px; color: #464545; font-weight: normal }"
"#code {font-size: 20px;}       "
 
"#menu{ width: 180px; float: right; text-transform: uppercase; margin-bottom: 50px;}    "
"#menu ul li{border-left: 4px solid #e93700; margin-top: 6px; padding: 2px 0 2px 4px; list-style:none; }"
"#menu ul li a{ font-size: 12px; color: #464545; text-decoration: none;}"
"#menu ul li a:hover{ color: #e93700 }"
"#menu ul{ margin:0; padding-left:80px; }"
"#menu h3 {color: #e93700; font-size: 18px; font-weight: normal; padding-left: 80px;}"

"#pie{ clear: both;  height: 296px; background: url(Images/logo_universite_bobo.jpg) no-repeat; background-position: bottom right;}"
"#boxCenterside{background: url(Images/logo_universite_bobo.jpg) repeat-x scroll center top transparent; min-height: 100px; width: 260px;}"


"</style>"

"<head>"
"<title>Juge</title>"
"</head>"
 
"<body>"
 
"<div id=\"cont\">"
	
"<div id=\"title\">"
"</div>"
"<div id=\"codeTitle\">"
"<h2>CODE SUBMITED</h2>"	
"<br>"
"</div>"	

"<div id=\"cajaarriba\">"
"</div>"
"<div id=\"code\">"
"<code>"
"<pre class='brush: cpp'>";

static const char *html_waiting_form2 =	
" </pre>"
"</code>"
"</div> "
"<div id=\"cajaabajo\">"
"</div>";

char sqlSentence[300];

//------------------make Submission interficie -------------------
void downloadPDF(struct mg_connection *conn, int problemId) {
	mg_printf_data(conn, " Download statement <form method=\"POST\" action=\"PROBLEMS/%i/problem.tar.gz\" enctype=\"multipart/form-data\"> "
	"<input type=\"submit\" value=\"Download\" /> <br> <br> </form>", problemId);
}

void SubmitC(struct mg_connection *conn, int idCourse, int problemId, int user) {
	mg_printf_data(conn, "C lenguage submission.<br>"
	"<form method=\"POST\" action=\"/submissionC\" "
	" enctype=\"multipart/form-data\">"
	"<input type=\"file\" name=\"file\" /> <br/>"
	"<input type=\"submit\" value=\"Submit\" />"
	"<br>"
	"<br> </form> ");	
}

void SubmitCPlusPlus(struct mg_connection *conn, int idCourse, int problemId, int user) {
	mg_printf_data(conn, "C++ lenguage submission.<br>"
	"<form method=\"POST\" action=\"/submissionP\" "
	" enctype=\"multipart/form-data\">"
	"<input type=\"file\" name=\"file\" /> <br/>"
	"<input type=\"submit\" value=\"Submit\" />"
	"<br>"
	"<br> </form> ");	
}


		

//----------------problem and cours information ----------------------------

int numberOfUsersInCoursThatSolvedTheProblemCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "Number of Uses that solved the problem: %s<br>", argv[0]);
	printf("%s\n", argv[0]);
	return 0;
}

void numberOfUsersInCoursThatSolvedTheProblem(struct mg_connection *conn, int idCourse, int problemId) {
	sprintf(sqlSentence, "SELECT COUNT(DISTINCT user_id) FROM submissions WHERE problem_id = %i AND course_id = %i AND result = 3;", problemId, idCourse);
	rc = sqlite3_exec(db, sqlSentence, numberOfUsersInCoursThatSolvedTheProblemCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int listOfUsersInCoursThatSolvedTheProblemCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "<li><a href=\"CourseId=%s&problemId=-1&userId=%s\">%s</a></li>", argv[2], argv[1], argv[0]);
	return 0;
}

void listOfUsersInCoursThatSolvedTheProblem(struct mg_connection *conn, int idCourse, int problemId) {
	sprintf(sqlSentence, "SELECT DISTINCT user_name, users.user_id, course_id FROM submissions, users WHERE submissions.user_id = users.user_id  AND problem_id = %i AND course_id = %i AND result = 3;", problemId, idCourse);
	rc = sqlite3_exec(db, sqlSentence, listOfUsersInCoursThatSolvedTheProblemCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

//------------------------------user and course information -------------------------------------------

int numberOfSubmissionsSendByUserCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "Number of Submissions made by the user: %s<br>", argv[0]);
	printf("%s\n", argv[0]);
	return 0;
}

void numberOfSubmissionsSendByUser(struct mg_connection *conn, int idCourse, int idUser) {
	sprintf(sqlSentence, "SELECT COUNT(id) FROM submissions where user_id = %i AND course_id = %i;", idUser, idCourse);
	rc = sqlite3_exec(db, sqlSentence, numberOfSubmissionsSendByUserCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int numberOfProblemsSolvedByUserCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "Number of Problems Solved by the user: %s<br>", argv[0]);
	printf("%s\n", argv[0]);
	return 0;
}

void numberOfProblemsSolvedByUser(struct mg_connection *conn, int idCourse, int idUser) {
	sprintf(sqlSentence, "SELECT COUNT(DISTINCT problem_id) FROM submissions where user_id = %i AND course_id = %i AND result = 3;", idUser, idCourse);
	rc = sqlite3_exec(db, sqlSentence, numberOfProblemsSolvedByUserCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int listOfProblemsSolvedByUserCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "<li><a href=\"CourseId=%s&problemId=%s\">%s</a></li>", argv[2], argv[1], argv[0]);
	return 0;
}

void listOfProblemsSolvedByUser(struct mg_connection *conn, int idCourse, int idUser) {
	sprintf(sqlSentence, "SELECT DISTINCT problem_name, problems.problem_id, course_id FROM submissions, problems WHERE submissions.problem_id = problems.problem_id AND user_id = %i AND course_id = %i AND result = 3;", idUser, idCourse);
	rc = sqlite3_exec(db, sqlSentence, listOfProblemsSolvedByUserCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}
//---------------------------Get from database ----------
			
int GetCoursNameFromIdCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "<html><body>Course name: %s <br><br>", argv[0]);
	printf("%s\n", argv[0]);
	return 0;
}
void GetCoursNameFromId(struct mg_connection *conn, int id)  {
	sprintf(sqlSentence, "SELECT course_name FROM courses WHERE course_id = %i;", id);
	rc = sqlite3_exec(db, sqlSentence, GetCoursNameFromIdCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int GetUserNameFromIdCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "User name: %s <br><br>", argv[0]);
	printf("%s\n", argv[0]);
	return 0;
}

void GetUserNameFromId(struct mg_connection *conn, int id)  {
	sprintf(sqlSentence, "SELECT user_name FROM users WHERE user_id = %i;", id);
	rc = sqlite3_exec(db, sqlSentence, GetUserNameFromIdCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int GetProblemNameFromIdCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "Problem name: %s <br><br>", argv[0]);
	printf("%s\n", argv[0]);
	return 0;
}

void GetProblemNameFromId(struct mg_connection *conn, int id)  {
	sprintf(sqlSentence, "SELECT problem_name FROM problems WHERE problem_id = %i;", id);
	rc = sqlite3_exec(db, sqlSentence, GetProblemNameFromIdCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

//---------------------------------------------
int numberOfProblemsCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "Number of problems: %s <br>", argv[0]);
	printf("%s\n", argv[0]);
	return 0;
}
void numberOfProblems(struct mg_connection *conn, int id)  {
	sprintf(sqlSentence, "SELECT COUNT(DISTINCT problem_id) FROM problemInCourse WHERE course_id = %i;", id);
	rc = sqlite3_exec(db, sqlSentence, numberOfProblemsCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int numberOfUsersCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "<br>Number of users: %s <br>", argv[0]);
	printf("%s\n", argv[0]);
	return 0;
}
void numberOfUsers(struct mg_connection *conn, int id)  {
	sprintf(sqlSentence, "SELECT COUNT(DISTINCT user_id) FROM userEnrolledInCourse where course_id = %i;", id);
	rc = sqlite3_exec(db, sqlSentence, numberOfUsersCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int listOfUsersCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "<li><a href=\"CourseId=%s&problemId=-1&userId=%s\">%s</a></li>", argv[2], argv[1], argv[0]);
	return 0;
}

void listOfUsers(struct mg_connection *conn, int id)  {
	sprintf(sqlSentence, "SELECT DISTINCT user_name, users.user_id, course_id FROM users, userEnrolledInCourse WHERE userEnrolledInCourse.user_id = users.user_id AND userEnrolledInCourse.course_id = %i", id);
	rc = sqlite3_exec(db, sqlSentence, listOfUsersCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int listOfProblemsCallBack2(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "<li><a href=\"SolveProblem2CourseId=%s&problemId=%s\">%s</a></li>", argv[2], argv[1], argv[0]);
	return 0;
}
void listOfProblems2(struct mg_connection *conn, int id)  {
	mg_printf_data(conn, "<html> <body> Select a problem <br>");
	sprintf(sqlSentence, "SELECT DISTINCT problem_name, problems.problem_id, course_id FROM problems, problemInCourse WHERE problemInCourse.problem_id = problems.problem_id AND problemInCourse.course_id = %i;", id);
	rc = sqlite3_exec(db, sqlSentence, listOfProblemsCallBack2, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int listOfProblemsCallBack3(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "<li><a href=\"checkSendedCode2=%s&problemId=%s\">%s</a></li>", argv[2], argv[1], argv[0]);
	return 0;
}
void listOfProblems3(struct mg_connection *conn, int id)  {
	mg_printf_data(conn, "<html> <body> Select a problem <br>");
	sprintf(sqlSentence, "SELECT DISTINCT problem_name, problems.problem_id, course_id FROM problems, problemInCourse WHERE problemInCourse.problem_id = problems.problem_id AND problemInCourse.course_id = %i;", id);
	rc = sqlite3_exec(db, sqlSentence, listOfProblemsCallBack3, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int listOfSolvedProblemsCallBack2(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "%s <br>", argv[0]);
	return 0;
}
void listOfSolvedProblems(struct mg_connection *conn, int idCourse, int idUser)  {
	mg_printf_data(conn, "<html> <body> <br><br> List of Problems Solved <br>");
	sprintf(sqlSentence, "SELECT DISTINCT problem_name FROM problems, problemInCourse, submissions WHERE submissions.problem_id = problems.problem_id AND problemInCourse.problem_id = problems.problem_id AND problemInCourse.course_id = %i AND submissions.user_id = %i AND submissions.result = 3;", idCourse, idUser);
	rc = sqlite3_exec(db, sqlSentence, listOfSolvedProblemsCallBack2, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}


int listOfProblemsCallBack(void *a_param, int argc, char **argv, char **column) {
	mg_printf_data(a_param, "<li><a href=\"CourseId=%s&problemId=%s\">%s</a></li>", argv[2], argv[1], argv[0]);
	return 0;
}
void listOfProblems(struct mg_connection *conn, int id)  {
	sprintf(sqlSentence, "SELECT DISTINCT problem_name, problems.problem_id, course_id FROM problems, problemInCourse WHERE problemInCourse.problem_id = problems.problem_id AND problemInCourse.course_id = %i;", id);
	rc = sqlite3_exec(db, sqlSentence, listOfProblemsCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int listOfAllCoursesCallBack(void *a_param, int argc, char **argv, char **column){
	mg_printf_data(a_param, "<li><a href=\"SolveProblem1CourseId=%s\">%s</a></li></body></html>", argv[1], argv[0]);
    return 0;
}


void listOfAllCourses(int user, struct mg_connection *conn) {
	mg_printf_data(conn, "<html><body>\n");
	sprintf(sqlSentence, "SELECT DISTINCT course_name, courses.course_id  FROM courses, userEnrolledInCourse WHERE user_id = %i AND courses.course_id=userEnrolledInCourse.course_id;", user);
	//printf("getUserID SQL = %s\n", sqlSentence);
	rc = sqlite3_exec(db, sqlSentence, listOfAllCoursesCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int listOfAllCoursesCallBack2(void *a_param, int argc, char **argv, char **column){
	mg_printf_data(a_param, "<li><a href=\"checkSendedCode1=%s\">%s</a></li></body></html>", argv[1], argv[0]);
    return 0;
}


void listOfAllCourses2(int user, struct mg_connection *conn) {
	mg_printf_data(conn, "<html><body>\n");
	sprintf(sqlSentence, "SELECT DISTINCT course_name, courses.course_id  FROM courses, userEnrolledInCourse WHERE user_id = %i AND courses.course_id=userEnrolledInCourse.course_id;", user);
	//printf("getUserID SQL = %s\n", sqlSentence);
	rc = sqlite3_exec(db, sqlSentence, listOfAllCoursesCallBack2, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}
int informationOfAllCoursesCallBack(void *a_param, int argc, char **argv, char **column){
	mg_printf_data(a_param, "<html><body><li><a href=\"CourseId=%s\">%s</a></li></body></html>", argv[1], argv[0]);
    return 0;
}


void informationOfAllCourses(int user, struct mg_connection *conn) {
	sprintf(sqlSentence, "SELECT DISTINCT course_name, courses.course_id  FROM courses, userEnrolledInCourse WHERE user_id = %i AND courses.course_id=userEnrolledInCourse.course_id;", user);
	//printf("getUserID SQL = %s\n", sqlSentence);
	rc = sqlite3_exec(db, sqlSentence, informationOfAllCoursesCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int ShowAllCoursesCallBack(void *a_param, int argc, char **argv, char **column){
	printf("course: == %s\n", argv[0]);
    return 0;
}

void ShowAllCourses() {
	sprintf(sqlSentence, "SELECT DISTINCT course_name FROM courses;");
	//printf("getUserID SQL = %s\n", sqlSentence);
	rc = sqlite3_exec(db, sqlSentence, ShowAllCoursesCallBack, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int identificator;
int getUserIdCallBack(void *a_param, int argc, char **argv, char **column){
	
	
	//printf("result sql == %s\n", argv[0]);
	identificator = atoi(argv[0]);
    return 0;
}

int getUserId (struct mg_connection *conn) { //this function gives you your userID
	char name[100], expire[100], ssid[100];
	mg_parse_header(mg_get_header(conn, "Cookie"), "ssid", ssid, sizeof(ssid));
	sscanf(ssid, "%[^|]|%[^|]|", name, expire);
	char command[300];
	sprintf(command, "select user_id from users where user_name='%s';", name);
	//printf("command= %s\n", command);
	char* ptr_command = command;
	char* error_get_table=NULL;
	char** result=NULL;
	int column=0;
	int row=0;
	 
	if (sqlite3_get_table(db, ptr_command,&result,&row,&column,&error_get_table))
	{
	   printf("SQLite Fehler: %s",error_get_table);
	   sqlite3_free(error_get_table);
	   error_get_table=NULL;
	   return -1;
	}
	
	if (row*column+row == 0) return -1; //no user with pass
	int id = atoi(result[1]);
	//printf("getUserID = %i", id);
	sqlite3_free_table(result);
	return id;
}

int maxIdProblem = 0;
int callbackMaxIdProblem(void *a_param, int argc, char **argv, char **column){ 
	maxIdProblem = 0;
	maxIdProblem = atoi(argv[0]);
	printf("maxIdProblem==%i\n", maxIdProblem);
	return 0;
}
void getLastIdProblemUsed() {
	sprintf(sqlSentence, "SELECT max(problem_id) FROM problems;");
	//printf("getUserID SQL = %s\n", sqlSentence);
	rc = sqlite3_exec(db, sqlSentence, callbackMaxIdProblem, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int getIdNewProblem(){
	getLastIdProblemUsed();
	return maxIdProblem+1;
}

int getUserType(int id) { //this function conects to the database and looks the type of user of the user id.	
	char command[300];
	sprintf(command, "select user_type from users where user_id='%i';", id);
	printf("command= %s\n", command);
	char* ptr_command = command;
	char* error_get_table=NULL;
	char** result=NULL;
	int column=0;
	int row=0;
	 
	if (sqlite3_get_table(db, ptr_command,&result,&row,&column,&error_get_table))
	{
	   printf("SQLite Fehler: %s",error_get_table);
	   sqlite3_free(error_get_table);
	   error_get_table=NULL;
	   return -1;
	}
	
	if (row*column+row == 0) return -1; 
	int ret = atoi(result[1]);
	sqlite3_free_table(result);
	return ret;
}

void makeSubmission(struct mg_connection *conn, char name[64], int userID, int courseID, int problemID, int problemResult)  {
	sprintf(sqlSentence, "insert into submissions(submission_name, user_id, course_id, problem_id, result) values('%s', %i, %i, %i, %i)",  name, userID, courseID, problemID, problemResult);
	printf("makeSubmission SQL = %s\n", sqlSentence);
	rc = sqlite3_exec(db, sqlSentence, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

void addProblemToDatabase(char var_name[100], int idProblem) {
	sprintf(sqlSentence, "insert into problems(problem_id, problem_name) values(%i, '%s');", idProblem, var_name);
	printf("makeSubmission SQL = %s\n", sqlSentence);
	rc = sqlite3_exec(db, sqlSentence, NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}
		
int canAccesResource(int user, struct mg_connection *conn) {
	int tipe = getUserType(user);
	printf("tipe = %i\n", tipe);
	char comparationUri[100] = "";

	if (tipe == 2) { //admin
		return 1;
	} else if (tipe == 1) { //professor
		return 1;
	} else if (tipe == 0) { //user
		//block acces to /SUBMISSIONS/BYUSER/id where id is difernet from the user id.
		if(strncmp(conn->uri, "/SUBMISSIONS/BYUSER/", strlen("/SUBMISSIONS/BYUSER/")) == 0) {
			sprintf(comparationUri, "/SUBMISSIONS/BYUSER/%i", user); 
			printf("comparationURI = %s, length = %i\n", comparationUri, strlen(comparationUri));
			if(strncmp(conn->uri, comparationUri, strlen(comparationUri)) == 0) {
				return 1;
			} else {
				return -1;
			}
		}
		
		//Block the acces of normla users to /SUBMISSIONS/BYIDPROBLEM
		sprintf(comparationUri, "/SUBMISSIONS/BYIDPROBLEM/"); 
		printf("comparationURI = %s, length = %i\n", comparationUri, strlen(comparationUri));
		if(strncmp(conn->uri, comparationUri, strlen(comparationUri)) == 0) {
			return -1;
		} 
		//Block acces to problems, the users only will have acces to the problems allowed in the database.
		/*sprintf(comparationUri, "/PROBLEMS/"); 
		printf("comparationURI = %s, length = %i\n", comparationUri, strlen(comparationUri));
		if(strncmp(conn->uri, comparationUri, strlen(comparationUri)) == 0) {
			return -1;
		} */
		//Block acces to look information option.
		sprintf(comparationUri, "/lookInformation"); 
		printf("comparationURI = %s, length = %i\n", comparationUri, strlen(comparationUri));
		if(strncmp(conn->uri, comparationUri, strlen(comparationUri)) == 0) {
			return -1;
		} 
		
		//Block acces to problems, the users only will have acces to the problems allowed in the database.
		sprintf(comparationUri, "/Course"); 
		printf("comparationURI = %s, length = %i\n", comparationUri, strlen(comparationUri));
		if(strncmp(conn->uri, comparationUri, strlen(comparationUri)) == 0) {
			return -1;
		} 
		
		//Block acces to problems, the users only will have acces to the problems allowed in the database.
		sprintf(comparationUri, "/addProblem"); 
		printf("comparationURI = %s, length = %i\n", comparationUri, strlen(comparationUri));
		if(strncmp(conn->uri, comparationUri, strlen(comparationUri)) == 0) {
			return -1;
		} 
		
		//Block acces to problems, the users only will have acces to the problems allowed in the database.
		sprintf(comparationUri, "/addProblem2"); 
		printf("comparationURI = %s, length = %i\n", comparationUri, strlen(comparationUri));
		if(strncmp(conn->uri, comparationUri, strlen(comparationUri)) == 0) {
			return -1;
		} 
		
		//Block acces to problems, the users only will have acces to the problems allowed in the database.
		sprintf(comparationUri, "/addProblemDatabase"); 
		printf("comparationURI = %s, length = %i\n", comparationUri, strlen(comparationUri));
		if(strncmp(conn->uri, comparationUri, strlen(comparationUri)) == 0) {
			return -1;
		} 
		
		return 1;
	}
	return -1;
}

//-------------------------------LOGIN-----------------------------------------------------------

static const char *s_login_uri = "/login.html";
static const char *s_secret = ":-)--()["; // Must be known only to server

static void generate_ssid(const char *user_name, const char *expiration_date,
char *ssid, size_t ssid_size) {
	char hash[33];
	mg_md5(hash, user_name, ":", expiration_date, ":", s_secret, NULL);
	snprintf(ssid, ssid_size, "%s|%s|%s", user_name, expiration_date, hash);
}

static int check_auth(struct mg_connection *conn) {
	char ssid[100], calculated_ssid[100], name[100], expire[100];
	// Always authenticate requests to login page
	if (strcmp(conn->uri, s_login_uri) == 0) {
		return MG_TRUE;
	}

	// Look for session ID in the Cookie.
	// That session ID can be validated against the database that stores
	// current active sessions.
	mg_parse_header(mg_get_header(conn, "Cookie"), "ssid", ssid, sizeof(ssid));
	if (sscanf(ssid, "%[^|]|%[^|]|", name, expire) == 2) {
		generate_ssid(name, expire, calculated_ssid, sizeof(calculated_ssid));
		if (strcmp(ssid, calculated_ssid) == 0) {
			return MG_TRUE; // Authenticate
		}
	}
	// Auth failed, do NOT authenticate, redirect to login page
	mg_printf(conn, "HTTP/1.1 302 Moved\r\nLocation: %s\r\n\r\n", s_login_uri);
	return MG_FALSE;
}	

static int check_login_form_submission(struct mg_connection *conn) {
	char name[100], password[100], ssid[100], expire[100], expire_epoch[100];
	
	mg_get_var(conn, "name", name, sizeof(name));
	mg_get_var(conn, "password", password, sizeof(password));
	
	if (validUserPassword(name, password) != -1) {
		printf("loged in, making cookie\n");
		// Generate expiry date
		time_t t = time(NULL) + 3600; // Valid for 1 hour
		snprintf(expire_epoch, sizeof(expire_epoch), "%lu", (unsigned long) t);
		strftime(expire, sizeof(expire), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&t));
		generate_ssid(name, expire_epoch, ssid, sizeof(ssid));
		// Set "session id" cookie, there could be some data encoded in it.
		mg_printf(conn,
		"HTTP/1.1 302 Moved\r\n"
		"Set-Cookie: ssid=%s; expire=\"%s\"; http-only; HttpOnly;\r\n"
		"Location: /\r\n\r\n",
		ssid, expire);
		mg_send_file(conn, "index.html", NULL);
		return MG_MORE;
	}
	return MG_FALSE;
}

int validUserPassword(char name[100],  char password[100]) {
	char command[300];
	sprintf(command, "select user_id from users where user_name='%s' and user_pass='%s';", name, password);
	printf("command= %s\n", command);
	char* ptr_command = command;
	char* error_get_table=NULL;
	char** result=NULL;
	int column=0;
	int row=0;
	 
	if (sqlite3_get_table(db, ptr_command,&result,&row,&column,&error_get_table))
	{
	   printf("SQLite Fehler: %s",error_get_table);
	   sqlite3_free(error_get_table);
	   error_get_table=NULL;
	   return -1;
	}
	
	if (row*column+row == 0) return -1; //no user with pass
	int id = atoi(result[1]);
	sqlite3_free_table(result);
	return id;
}

int nameAvaiable(char var_name[40]) {
	char command[300];
	sprintf(command, "select problem_name from problems where problem_name ='%s';", var_name);
	printf("command= %s\n", command);
	char* ptr_command = command;
	char* error_get_table=NULL;
	char** result=NULL;
	int column=0;
	int row=0;
	 
	if (sqlite3_get_table(db, ptr_command,&result,&row,&column,&error_get_table))
	{
	   printf("SQLite Fehler: %s",error_get_table);
	   sqlite3_free(error_get_table);
	   error_get_table=NULL;
	   return -1;
	}
	
	int res = -1;
	if (row*column+row == 0) res = 1;
	sqlite3_free_table(result);
	return res;
}

int showSubmissionProblemCallBack(void *a_param, int argc, char **argv, char **column){
	//mg_printf_data(a_param, "%s, result = %s\n", argv[0], argv[1]);
	mg_printf_data(a_param, "<html><body> <br> <br> <li><a href=\"showSubmission=%s\"> %s </a></li></body></html>", argv[0], argv[0]);
	if (strcmp(argv[1], "3") == 0) {
		mg_printf_data(a_param, "Problem solved in this submission. \n");
	
	} else if (strcmp(argv[1], "2") == 0) {
		mg_printf_data(a_param, "You exceded the execution time \n");
	
	} else if (strcmp(argv[1], "1") == 0) {
		mg_printf_data(a_param, "Error passing the tests, your answer is wrong for at least one test.\n\n");
	
	} else if (strcmp(argv[1], "0") == 0) {
		mg_printf_data(a_param, "Compilation Error.\n\n");
	
	} else if (strcmp(argv[1], "-1") == 0) {
		mg_printf_data(a_param, "The system cannot evaluate your solution cause there is and error on the problem design, please contact the admin.\n\n");
	}
	
    return 0;
}

void showSubmissionProblem(struct mg_connection *conn, int idCourse, int problemID, int userID) {
	sprintf(sqlSentence, "SELECT submission_name, result FROM submissions WHERE course_id = %i AND problem_id = %i AND user_id = %i;",  idCourse, problemID, userID);
	printf("makeSubmission SQL = %s\n", sqlSentence);
	rc = sqlite3_exec(db, sqlSentence, showSubmissionProblemCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

int showAllSubmissionsUserCallBack(void *a_param, int argc, char **argv, char **column){
	//mg_printf_data(a_param, "%s, result = %s\n", argv[0], argv[1]);
	mg_printf_data(a_param, "Problem: %s............ Veredict:", argv[1]);
	if (strcmp(argv[2], "3") == 0) {
		mg_printf_data(a_param, "SOLVED. \n");
	
	} else if (strcmp(argv[2], "2") == 0) {
		mg_printf_data(a_param, "You exceded the execution time \n");
	
	} else if (strcmp(argv[2], "1") == 0) {
		mg_printf_data(a_param, "Error passing the tests, your answer is wrong for at least one test.\n\n");
	
	} else if (strcmp(argv[2], "0") == 0) {
		mg_printf_data(a_param, "Compilation Error.\n\n");
	
	} else if (strcmp(argv[2], "-1") == 0) {
		mg_printf_data(a_param, "The system cannot evaluate your solution cause there is and error on the problem design, please contact the admin.\n\n");
	}
	mg_printf_data(a_param, "<html><body> <li><a href=\"showSubmission=%s\"> %s </a></li> <br> <br>", argv[0], argv[0]);
	
    return 0;
}

void showAllSubmissionsUser(struct mg_connection *conn, int idCourse, int userID) {
	sprintf(sqlSentence, "SELECT submission_name, problem_name, result FROM submissions, problems WHERE course_id = %i AND user_id = %i AND problems.problem_id = submissions.problem_id;",  idCourse, userID);
	printf("makeSubmission SQL = %s\n", sqlSentence);
	rc = sqlite3_exec(db, sqlSentence, showAllSubmissionsUserCallBack, conn, &zErrMsg);
	if( rc!=SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

//-------------------------------FINAL LOGIN-----------------------------------------------------------

char *trim(char *s)
{
  char *start = s;
  while(*start && isspace(*start))
    ++start;
  char *i = start;
  char *end = start;

  while(*i)
  {
    if( !isspace(*(i++)) )
      end = i;
  }
  *end = 0;
  return start;
}

int getCompiler(int idProblem) { //given the id of the problem, this function gets the value of the variables needPassAll,
							//numberPublicTest, and numberPrivateTest of the problem.
							//If the folder with the id of the problem dosen't exist, then it returns -1.
	xmlDoc *document;
	xmlNode *root, *first_child, *node;
	xmlChar *value;
	int compilerSample;
	char line[128];
	sprintf(line, "./PROBLEMS/%i/info.xml", idProblem);
	FILE *fp;
	fp = fopen(line, "r");
	if(fp == NULL) {
		return -1;
	}
	fclose(fp);
	document = xmlReadFile(line, NULL, 0);
	root = xmlDocGetRootElement(document);
	
	first_child = root->children;
	for (node = first_child; node; node = node->next) {
		value=xmlNodeGetContent(node);
		
		if (strcmp(node->name, "compilerSample") == 0) {
			compilerSample = atoi(trim((char*)value));
		} 
    }
    printf("compilador del problem = %i\n", compilerSample);
	return compilerSample;
}


static int serve_request(struct mg_connection *conn) {
	
	if (strcmp(conn->uri, s_login_uri) == 0 &&
	strcmp(conn->request_method, "POST") == 0) {
		return check_login_form_submission(conn);
	}
	const char *data;
	int data_len;
	char var_name[100], file_name[100], path[100];
	int userID = getUserId(conn);
	//printf("Connection infomration== %s\n", conn->content);
	if (userID != -1 && canAccesResource(userID, conn) == 1) {
		if (strcmp(conn->uri, "/checkSendedCode") == 0) {
			listOfAllCourses2(userID, conn);
			return MG_TRUE;
	 
		} else if (strncmp(conn->uri, "/checkSendedCode1", 17) == 0) {
			int idCourse = -1;
			int problemId = -1;
			int idUser = -1;
			
			int size = strlen(conn->uri);
			char read[size-7];
			memset(read, 'j', sizeof(read));
			int i;
			int positionRead = 0;
			for (i = 17; i < size; i++) {
				if(conn->uri[i] == '=') {
					i++;
					int j = i;
					while (conn->uri[j] > '0' && conn->uri[j] < '9') {
						read[positionRead] = conn->uri[j];
						positionRead++;
						j++;
					}
					idCourse = atoi(read);
					memset(read, 'j', sizeof(read));
					positionRead = 0;
				}
			}
			listOfProblems3(conn, idCourse);
			mg_printf_data(conn, "</html> </body>");
			return MG_TRUE; // Tell mongoose to keep this connection open
			
		} else if (strncmp(conn->uri, "/checkSendedCode2", 17) == 0) {
			int idCourse = -1;
			int problemId = -1;
			
			char courseID[15];
			char problemID[15];
			int size = strlen(conn->uri);
			char read[size-17];
			memset(read, 'j', sizeof(read));
			int i;
			int variablesRead = 0;
			int positionRead = 0;
			for (i = 17; i < size; i++) {
				if(conn->uri[i] == '=') {
					i++;
					int j = i;
					while (conn->uri[j] > '0' && conn->uri[j] < '9') {
						read[positionRead] = conn->uri[j];
						positionRead++;
						j++;
					}
					i = j;
					
					if (variablesRead == 0) {
						idCourse = atoi(read);
						sprintf(courseID, "%i\n", idCourse);
						
					} else if (variablesRead == 1) {
						problemId = atoi(read);
						sprintf(problemID, "%i\n", problemId);
					} 
					memset(read, 'j', sizeof(read));
					positionRead = 0;
					variablesRead++;
				}
			}
			showSubmissionProblem(conn, idCourse, problemId, userID);
			return MG_TRUE;
		}else if (strncmp(conn->uri, "/showSubmission", 15) == 0) {

			int size = strlen(conn->uri);
			char read[size];	
			int positionRead = 0;
			int i;
			int fi = 0;
			int aux = 0;
			for (i = 15; i < size && fi == 0; i++) {
				if(conn->uri[i] == '=') {
					i++;
					read[positionRead] = conn->uri[i];
					positionRead++;
				} else if (positionRead > 0) {
					read[positionRead] = conn->uri[i];
					positionRead++;
					if (conn->uri[i] == '-') aux++;
					if (aux == 8) {
						fi = 1;
						i++;
						read[positionRead] = conn->uri[i];
						printf("positionRead = %i\n", positionRead);
					}
				}		
			}
			char readClean[positionRead+1];
			strncpy(readClean, read, positionRead+1);	
			//printf("Uri = %s. S'han llegit el nom de submissio seguent: %s\n", conn->uri, readClean);
			
			mg_printf_data(conn, "<html><body><br><li><a href=\"/SUBMISSIONS/BYUSER/%i/%s.result.txt\">Result</a></li>", userID, readClean);
			mg_printf_data(conn, "<br><li><a href=\"/SUBMISSIONS/BYUSER/%i/%s.code.txt\">Code</a></li></html></body>", userID, readClean);
			return MG_TRUE;
		
		} else if (strcmp(conn->uri, "/addProblem") == 0) {
			mg_printf_data(conn, "%s",
			"<html><body>Add your tar.gz file with the format specificated to add a problem."
			"<br> <br> <form method=\"POST\" action=\"/addProblem2\" "
			" enctype=\"multipart/form-data\">"
			"<input type=\"file\" name=\"file\" /> <br/><br> <br>"
			"<input type=\"submit\" value=\"Upload\" />"
			"</form></body></html>");
			return MG_TRUE; // Tell mongoose to close this connectionelse {
		  
		} else if (strcmp(conn->uri, "/addProblem2") == 0) {
			mg_send_data(conn, html_problem_add, strlen(html_problem_add));
			if (mg_parse_multipart(conn->content, conn->content_len, var_name, sizeof(var_name), file_name, sizeof(file_name), &data, &data_len) > 0) {
				struct file_data *p = (struct file_data *) malloc(sizeof(*p));
				snprintf(path, sizeof(path), "P_%i.tar.gz", userID);
				printf("el path es ->>> %s\n", path);
				p->fp = fopen(path, "wb");
				p->data = data;
				p->data_len = data_len;
				p->written = 0;
				conn->connection_param = p;
				mg_send_header(conn, "Content-Type", "text/html");
			}
			return MG_MORE; // Tell mongoose to keep this connection open
		} else if (strcmp(conn->uri, "/createCourse") == 0) {
			//Shows all names of all the courses in the system
			ShowAllCourses();
			
			mg_printf_data(conn, "%s",
			"<html><body>Name of the create Course"
			"<form method=\"POST\" action=\"/createCourse2\" "
			"enctype=\"multipart/form-data\">"
			"<input type=\"text\" name=\"course_name\" /> <br/>"
			"<input type=\"submit\" value=\"Create Course\" />"
			"<br>"
			"</form></body></html>");
			return MG_TRUE; // Tell mongoose to keep this connection open

		} else if (strcmp(conn->uri, "/lookInformation") == 0) {
			//Shows all names of all the courses in the system
			informationOfAllCourses(userID, conn);
			return MG_TRUE; // Tell mongoose to keep this connection open

		} else if (strncmp(conn->uri, "/Course", 7) == 0) {
			int idCourse = -1;
			int problemId = -1;
			int idUser = -1;
			
			int size = strlen(conn->uri);
			char read[size-7];
			memset(read, 'j', sizeof(read));
			int i;
			int variablesRead = 0;
			int positionRead = 0;
			for (i = 6; i < size; i++) {
				if(conn->uri[i] == '=') {
					i++;
					int j = i;
					if(conn->uri[j] == '-') {
						if (variablesRead == 0) {
							idCourse = -1;
						} else if (variablesRead == 1) {
							problemId = -1;
						} else if (variablesRead == 2) {
							idUser = -1;
						}
						variablesRead++;
						i++;
					} else {
						while (conn->uri[j] > '0' && conn->uri[j] < '9') {
							read[positionRead] = conn->uri[j];
							positionRead++;
							j++;
						}
						i = j;
						
						if (variablesRead == 0) {
							idCourse = atoi(read);
							
						} else if (variablesRead == 1) {
							problemId = atoi(read);
							
						} else if (variablesRead == 2) {
							idUser = atoi(read);
						}
						memset(read, 'j', sizeof(read));
						positionRead = 0;
						variablesRead++;
					}
				}
			}
			//printf("S'han llegit les dades del curs seguëunts: idCourse = %i,  problemId = %i, idUser = %i\n", idCourse, problemId, idUser);
			
			if (idCourse != -1 && problemId == -1 && idUser == -1) { //Course
			
				GetCoursNameFromId(conn, idCourse);
				numberOfProblems(conn, idCourse);
				listOfProblems(conn, idCourse);
				numberOfUsers(conn, idCourse);
				listOfUsers(conn, idCourse);
			} else if (idCourse != -1 && problemId != -1 && idUser == -1) { //Course and Problem
				GetCoursNameFromId(conn, idCourse);
				GetProblemNameFromId(conn, problemId);
				numberOfUsersInCoursThatSolvedTheProblem(conn, idCourse, problemId);
				listOfUsersInCoursThatSolvedTheProblem(conn, idCourse, problemId);
				mg_printf_data(conn, "</body> </html>");

			} else if (idCourse != -1 && problemId == -1 && idUser != -1) { //Course and User
				GetCoursNameFromId(conn, idCourse);
				GetUserNameFromId(conn, idUser);
				numberOfSubmissionsSendByUser(conn, idCourse, idUser);
				numberOfProblemsSolvedByUser(conn, idCourse, idUser);
				listOfProblemsSolvedByUser(conn, idCourse, idUser);
				mg_printf_data(conn, "<br> <br> Submission made by the user: <br> <br> ");
				showAllSubmissionsUser(conn, idCourse, idUser);
				
				mg_printf_data(conn, "</body> </html>");
			}
			return MG_TRUE; // Tell mongoose to keep this connection open

		} else if (strcmp(conn->uri, "/SolveProblem") == 0) {
			//Shows all names of all the courses in the system
			listOfAllCourses(userID, conn);
			return MG_TRUE; // Tell mongoose to keep this connection open

		} else if (strncmp(conn->uri, "/SolveProblem1", 14) == 0) {
			int idCourse = -1;
			int problemId = -1;
			int idUser = -1;
			
			int size = strlen(conn->uri);
			char read[size-7];
			memset(read, 'j', sizeof(read));
			int i;
			int positionRead = 0;
			for (i = 13; i < size; i++) {
				if(conn->uri[i] == '=') {
					i++;
					int j = i;
					while (conn->uri[j] > '0' && conn->uri[j] < '9') {
						read[positionRead] = conn->uri[j];
						positionRead++;
						j++;
					}
					idCourse = atoi(read);
					memset(read, 'j', sizeof(read));
					positionRead = 0;
				}
			}
			listOfProblems2(conn, idCourse);
			listOfSolvedProblems(conn, idCourse, userID);
			mg_printf_data(conn, "</html> </body>");
			return MG_TRUE; // Tell mongoose to keep this connection open
			
		} else if (strncmp(conn->uri, "/SolveProblem2", 14) == 0) {
			int idCourse = -1;
			int problemId = -1;
			
			char courseID[15];
			char problemID[15];
			int size = strlen(conn->uri);
			char read[size-7];
			memset(read, 'j', sizeof(read));
			int i;
			int variablesRead = 0;
			int positionRead = 0;
			for (i = 6; i < size; i++) {
				if(conn->uri[i] == '=') {
					i++;
					int j = i;
					while (conn->uri[j] > '0' && conn->uri[j] < '9') {
						read[positionRead] = conn->uri[j];
						positionRead++;
						j++;
					}
					i = j;
					
					if (variablesRead == 0) {
						idCourse = atoi(read);
						sprintf(courseID, "%i\n", idCourse);
						
					} else if (variablesRead == 1) {
						problemId = atoi(read);
						sprintf(problemID, "%i\n", problemId);
					} 
					memset(read, 'j', sizeof(read));
					positionRead = 0;
					variablesRead++;
				}
			}
			printf("S'han llegit les dades del curs seguëunts: idCourse = %i,  problemId = %i\n", idCourse, problemId);

			char name[30];
			sprintf(name, "info%i.txt", userID);
			FILE *fp;
			fp = fopen(name, "w");
			if (fp!=NULL)
			{
				fputs (courseID,fp);
				fputs (problemID,fp);
				fclose (fp);
			}
			
			mg_printf_data(conn, "<html> <body>");
			GetProblemNameFromId(conn, problemId);
			downloadPDF(conn, problemId);
			SubmitC(conn, idCourse, problemId, userID);
			SubmitCPlusPlus(conn, idCourse, problemId, userID);
			return MG_TRUE; // Tell mongoose to keep this connection open

		} else if (strcmp(conn->uri, "/submissionC") == 0) {
			mg_send_data(conn, html_waiting_form1, strlen(html_waiting_form1));
			if (mg_parse_multipart(conn->content, conn->content_len, var_name, sizeof(var_name), file_name, sizeof(file_name), &data, &data_len) > 0) {
				struct file_data *p = (struct file_data *) malloc(sizeof(*p));
				snprintf(path, sizeof(path), "S_%i", userID);
				char name[20];
				sprintf(name, "info%i.txt", userID);
				FILE *fp;
				fp = fopen(name, "a");
				if (fp!=NULL)
				{
					fputs ("C\n",fp);
					fclose (fp);
				}
				printf("el path es ->>> %s\n", path);
				p->fp = fopen(path, "wb");
				p->data = data;
				p->data_len = data_len;
				p->written = 0;
				conn->connection_param = p;
				mg_send_header(conn, "Content-Type", "text/html");
			}
			return MG_MORE; // Tell mongoose to keep this connection open
		
		} else if (strcmp(conn->uri, "/submissionP") == 0) {
			mg_send_data(conn, html_waiting_form1, strlen(html_waiting_form1));
			if (mg_parse_multipart(conn->content, conn->content_len, var_name, sizeof(var_name), file_name, sizeof(file_name), &data, &data_len) > 0) {
				struct file_data *p = (struct file_data *) malloc(sizeof(*p));
				snprintf(path, sizeof(path), "S_%i", userID);
				char name[20];
				sprintf(name, "info%i.txt", userID);
				FILE *fp;
				fp = fopen(name, "a");
				if (fp!=NULL)
				{
					fputs ("P\n",fp);
					fclose (fp);
				}
				printf("el path es ->>> %s\n", path);
				p->fp = fopen(path, "wb");
				p->data = data;
				p->data_len = data_len;
				p->written = 0;
				conn->connection_param = p;
				mg_send_header(conn, "Content-Type", "text/html");
			}
			return MG_MORE; // Tell mongoose to keep this connection open	
		} else if (strcmp(conn->uri, "/addProblemDatabase") == 0) {		
			char idProblem[20];
			mg_get_var(conn, "nameProblem", var_name, sizeof(var_name));
			mg_get_var(conn, "id", idProblem, sizeof(idProblem));
			if (nameAvaiable(var_name) == 1) {
				addProblemToDatabase(var_name, atoi(idProblem));
				mg_printf_data(conn,
				"<html><body> <h2> Problem added correctly</h2>"
				"</body></html>");
				return MG_TRUE;
			}else {
				mg_printf_data(conn,
				"<html><body> <h2> Add problem name to finish the addition.</h2>"
				"<form method=\"POST\" action=\"addProblemDatabase\">"
				"<input type=\"text\" name=\"nameProblem\" />"
				"<input type=\"hidden\" name=\"id\" value=\"%s\" />"
				"<input type=\"submit\" value=\"Add name to problem\" />"
				"</form></body></html>", idProblem);
				return MG_TRUE;
			}
			return MG_TRUE; // Tell mongoose to keep this connection open
		} else { //if user can acces to the resource MG_FALSE, if not MG_TRUE
			printf("uri = %s\n",conn->uri);
			return MG_FALSE;
		}
	}
	if (strcmp(conn->uri, s_login_uri) == 0) {
		return MG_FALSE;
	}
	return MG_TRUE;
}

static int handle_poll(struct mg_connection *conn) {
	int userID = getUserId(conn);
	int courseID;
	int problemID;
	int compiler;
	char line[256];
	char name[256];
	int problemResult = -1;
	FILE *pipe1, *pipe;
	char line1[128];
	int linenr1, linenr;
	struct file_data *p = (struct file_data *) conn->connection_param;
	if (p != NULL) {
		// Write no more then 100 bytes in one go
		int len = p->data_len - p->written;
		int n = fwrite(p->data + p->written, 1, len > 100 ? 100 : len, p->fp);
		if (n > 0) {
			p->written += n;
			mg_send_data(conn, " ", 1); // Send something back to wake up select()
		}

		// If everything is written, close the connection
		if (p->written >= p->data_len) {
			//mg_printf_data(conn, "Written %d bytes, waiting the result...", p->written);
			fclose(p->fp);
			free(p);
			conn->connection_param = NULL;
			int added = 0;
			int type = getUserType(userID); 
			if (type == 1 || type == 2) {
				int idProblemAdded = getIdNewProblem();
				sprintf(name, "P_%i.tar.gz", userID);
				FILE *fp;
				fp = fopen(name, "r");
				if (fp!=NULL)
				{
					added = 1;
					if (idProblemAdded != -1) {
						sprintf(line, "./unzip %s %i", name, idProblemAdded);
						pipe = popen(line, "r");
						if (pipe == NULL) {  
							perror("Error onpip opening for the add problem operation()\n");
							return 1;        // return with exit code indicating error 
						}
						
						linenr = 1;
						while (fgets(line, 128, pipe) != NULL) {
							printf("Script output line %d: %s", linenr, line);
							++linenr;
						}
						pclose(pipe);
						
						remove(name);	
						//do test
						int compiler = getCompiler(idProblemAdded);
						printf("compiler = %i\n", compiler);
								
						int Socket_Manager_Client;
						int Socket_Worker_Client;

						char id[maxLengthIdWorker] = "";
						char ip[maxLengthIpNames] = "";
						char port[maxLengthPortNames] = "";

						int i;
						int ret;
						int messageCode = 0;
						char nameFileRecieved[100];

						ret=fork();
						if (ret > 0) {
							//sleep(1);
						} else if (ret == 0) {  //child process
							Socket_Manager_Client = Cliente_Abre_Conexion_Inet("localhost", "pclient");
							if (Socket_Manager_Client == 1)
							{
								printf ("CLIENT:Error connecting with the manager\n");
								perror("Error on connecting the socket");
							}

							SendMessage(Socket_Manager_Client, 1, "", "", "");
							int finished = 0;
							while (messageCode != 6 || finished == 0) {

								ReceiveMessage(Socket_Manager_Client, &messageCode, id, ip, port);
								sprintf(nameFileRecieved, "%s.txt", id);
								if(messageCode == 2) { //QUEUE FULL 
									//printf("The queue of petitions its full, PID =%i cannot be served, try again later \n", getpid());
									mg_printf_data(conn, "The queue of petitions its full, PID =%i cannot be served, try again later \n", getpid());
									finished = 1;
								}else if(messageCode == 3) {//QUEUED	
									//printf("All the workers are full, but the petition is stored in the queue, PID =%i will be served, but beofre there are %s petitions\n", getpid(), id); 
									mg_printf_data(conn, "All the workers are full, but the petition is stored in the queue, PID =%i will be served, but beofre there are %s petitions\n", getpid(), id);
								} else if (messageCode == 6) {
									printf("6 recieved, ip = %s, port = %s, iWorker = %s", ip, port, id);
									close(Socket_Manager_Client);
									Socket_Worker_Client = Cliente_Abre_Conexion_Inet(ip, port);
									if (Socket_Worker_Client == 1)
									{
										printf ("No puedo establecer conexion con el worker\n");
										exit (-1);
									}
									
									printf("PULL!! REbut courseID = %i, problemID = %i, compiler = %i\n", courseID, problemID, compiler);
									if (compiler == 0) {
										sprintf(name, "./PROBLEMS/%i/correct.c", idProblemAdded);
										SendFile(Socket_Worker_Client, idProblemAdded, compiler+2, name, -1);
									}else if (compiler == 1) {
										sprintf(name, "./PROBLEMS/%i/correct.cpp", idProblemAdded);
										SendFile(Socket_Worker_Client, idProblemAdded, compiler+2, name, -1);
									}
									ReceiveMessage(Socket_Manager_Client, &messageCode, id, ip, port);
									if (messageCode == 8) {//REQUEST PROBLEM
										printf("SERVER: REQUEST PROBLEM received on server idProblem = %s\n", id);
										char test[128];
										sprintf(test, "nc -w 3 %s 1234 < ./PROBLEMS/%s/tests.tar.gz", ip, id);
										pipe1 = popen(test, "r");
										if (pipe1 == NULL) {  // check for errors 
											perror("Error onpip opening for the add problem operation()\n");
											return 1;        // return with exit code indicating error 
										}
										
										linenr1 = 1;
										while (fgets(test, 128, pipe1) != NULL) {
											printf("Script output line %d: %s", linenr1, test);
											++linenr1;
										}
										pclose(pipe1);
										printf("file sended\n");
										int compiler, idProblem;
										RecieveFile(Socket_Worker_Client, &problemResult, &compiler); 
										close(Socket_Worker_Client);
										
									} else if (messageCode == 9) { //OK
										printf("SERVER:OK received on server\n");
										int compiler, idProblem;
										RecieveFile(Socket_Worker_Client, &problemResult, &compiler); 
										close(Socket_Worker_Client);
										
									}
									printf("problem result with value = %i\n", problemResult);
									if (problemResult == 3 || problemResult == 2) {
										mg_printf_data(conn,
										"<html><body> Add problem name to finish the addition."
										"<form method=\"POST\" action=\"addProblemDatabase\">"
										"<input type=\"text\" name=\"nameProblem\" />"
										"<input type=\"hidden\" name=\"id\" value=\"%i\" />"
										"<input type=\"submit\" value=\"Add name to problem\" />"
										"</form></body></html>", idProblemAdded);
										return MG_TRUE;
									} else {
										sprintf(line1, "./deleteProblemFromServer %i", idProblemAdded);
										printf("comand: %s\n", line1);
										pipe1 = popen(line1, "r");
										if (pipe1 == NULL) {  
											perror("Error onpip opening for the add problem operation()\n");
											return 1;        //return with exit code indicating error 
										}
										linenr1 = 1;
										while (fgets(line1, 128, pipe1) != NULL) {
											printf("Script output line %d: %s", linenr1, line1);
											++linenr1;
										}
										pclose(pipe1);
										mg_printf_data(conn, "%s", "<br> <br> There are problems with the problem you submitted, are you sure that it is well designed?");
										return MG_TRUE;
									}
	
									return MG_FALSE;
								}
							}

						} else if(ret == -1) {
							perror("Error on fork()\n");
							exit(EXIT_FAILURE);
						} 	


						//------------------------------------------------

						return MG_FALSE; // Tell mongoose to close this connection
					}
						
					return MG_TRUE;
				} else {
					remove(name);
					mg_printf_data(conn, "%s", "Your problem has not been added. Probably failed due to your problem name, the name of the problem must be unique");
					return MG_TRUE;
				}
			}
			if (added == 0) {
				sprintf(name, "info%i.txt", userID);
				FILE *fp;
				fp = fopen(name, "r");
				if (fp!=NULL)
				{
					fgets(line, sizeof(line), fp);
					courseID = atoi(line);
					fgets(line, sizeof(line), fp);
					problemID = atoi(line);
					while (fgets(line, sizeof(line), fp)) {
						if (line[0] == 'C') {
							compiler = 0;
						}else {
							compiler = 1;
						}
					}
					fclose (fp);
				}
		//------------------------------------------Print Code Submited//
				sprintf(name, "S_%i", userID);
				FILE* file = fopen(name, "r"); 
				if (file == NULL) {
					exit(1);
				}

				while (fgets(line, sizeof(line), file)) {

					mg_printf_data(conn, "%s", line); 
					//printf("%s", line);
					//mg_send_data(conn, line, sizeof(line));
					//mg_printf_data(conn, "<br>");

				}
				fclose(file);
				mg_send_data(conn, html_waiting_form2, strlen(html_waiting_form2));
				//---------------------------------- backups on the submission directori
				time_t t = time(NULL);
				struct tm tm = *localtime(&t);
				char date[50];
				sprintf(date, "%d-%d-%d-time-%d-%d-%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				sprintf(line1, "./submissionCopyOnFolders %s %s %i %i", name, date, userID,  problemID);
				printf("comand: %s\n", line1);
				pipe1 = popen(line1, "r");
				if (pipe1 == NULL) {  
					perror("Error onpip opening for the add problem operation()\n");
					return 1;        //return with exit code indicating error 
				}
				linenr1 = 1;
				while (fgets(line1, 128, pipe1) != NULL) {
					printf("Script output line %d: %s", linenr1, line1);
					++linenr1;
				}
				pclose(pipe1);
				
				//--------------------------------Compilation process
				int Socket_Manager_Client;
				int Socket_Worker_Client;

				char id[maxLengthIdWorker] = "";
				char ip[maxLengthIpNames] = "";
				char port[maxLengthPortNames] = "";

				int i;
				int ret;
				int messageCode = 0;
				char nameFileRecieved[100];

				ret=fork();
				if (ret > 0) {
					//sleep(1);
				} else if (ret == 0) {  //child process

					Socket_Manager_Client = Cliente_Abre_Conexion_Inet("localhost", "pclient");
					if (Socket_Manager_Client == 1)
					{
						printf ("CLIENT:Error connecting with the manager\n");
						perror("Error on connecting the socket");
					}

					SendMessage(Socket_Manager_Client, 1, "", "", "");
					int finished = 0;
					while (messageCode != 6 || finished == 0) {

						ReceiveMessage(Socket_Manager_Client, &messageCode, id, ip, port);
						sprintf(nameFileRecieved, "%s.txt", id);
						if(messageCode == 2) { //QUEUE FULL 
							//printf("The queue of petitions its full, PID =%i cannot be served, try again later \n", getpid());
							mg_printf_data(conn, "The queue of petitions its full, PID =%i cannot be served, try again later \n", getpid());
							finished = 1;
						}else if(messageCode == 3) {//QUEUED	
							//printf("All the workers are full, but the petition is stored in the queue, PID =%i will be served, but beofre there are %s petitions\n", getpid(), id); 
							mg_printf_data(conn, "All the workers are full, but the petition is stored in the queue, PID =%i will be served, but beofre there are %s petitions\n", getpid(), id);
						} else if (messageCode == 6) {
							printf("6 recieved, ip = %s, port = %s, iWorker = %s", ip, port, id);
							close(Socket_Manager_Client);
							Socket_Worker_Client = Cliente_Abre_Conexion_Inet(ip, port);
							if (Socket_Worker_Client == 1)
							{
								printf ("No puedo establecer conexion con el worker\n");
								exit (-1);
							}
							
							printf("PULL!! REbut courseID = %i, problemID = %i, compiler = %i\n", courseID, problemID, compiler);
							if (compiler == 0) {
								//sprintf(name, "%s.c", name);
								SendFile(Socket_Worker_Client, problemID, compiler, name, -1);
							}else if (compiler == 1) {
								//sprintf(name, "%s.cpp", name);
								SendFile(Socket_Worker_Client, problemID, compiler, name, -1);
							}
							ReceiveMessage(Socket_Manager_Client, &messageCode, id, ip, port);
							if (messageCode == 8) {//REQUEST PROBLEM
								printf("SERVER: REQUEST PROBLEM received on server idProblem = %s\n", id);
								char test[128];
								sprintf(test, "nc -w 3 %s 1234 < ./PROBLEMS/%s/tests.tar.gz", ip, id);
								pipe1 = popen(test, "r");
								if (pipe1 == NULL) {  // check for errors 
									perror("Error onpip opening for the add problem operation()\n");
									return 1;        // return with exit code indicating error 
								}
								
								linenr1 = 1;
								while (fgets(test, 128, pipe1) != NULL) {
									printf("Script output line %d: %s", linenr1, test);
									++linenr1;
								}
								pclose(pipe1);
								printf("file sended\n");
								int compiler, idProblem;
								RecieveFile(Socket_Worker_Client, &problemResult, &compiler); 
								close(Socket_Worker_Client);
								
							} else if (messageCode == 9) { //OK
								printf("SERVER:OK received on server\n");
								int compiler, idProblem;
								RecieveFile(Socket_Worker_Client, &problemResult, &compiler); 
								close(Socket_Worker_Client);
								
							}
							printf("problem result with value = %i\n", problemResult);
							sprintf(name, "%s-%i-%i", date, userID, problemID);
							//printf(name);
							makeSubmission(conn, name, userID, courseID, problemID, problemResult);
							//-------------------------------saving result%s.txt on the correct folder -------(id worker)
							char line2[128];
							sprintf(line2, "./resultCopyOnFolders %s %s %i %i",  nameFileRecieved, date, userID,  problemID);
							pipe1 = popen(line2, "r");
							if (pipe1 == NULL) {  //check for errors 
								perror("Error onpip opening for the add problem operation()\n");
								return 1;        // return with exit code indicating error 
							}
							
							linenr1 = 1;
							while (fgets(line2, 128, pipe1) != NULL) {
								printf("Script output line %d: %s", linenr1, line2);
								++linenr1;
							}
							pclose(pipe1);
							//-----------------------------------------------
							mg_printf_data(conn,
							"<br>"
							"<html><body> <h2> View Result.</h2>"
							"<form method=\"POST\" action=\"%s\">"
							"<input type=\"submit\" value=\"To Result\" />"
							"</form></body></html>", nameFileRecieved);
							return MG_FALSE;
						}
					}

				} else if(ret == -1) {
					perror("Error on fork()\n");
					exit(EXIT_FAILURE);
				} 	


				//------------------------------------------------

				return MG_FALSE; // Tell mongoose to close this connection
			}
		}
	}
	return MG_FALSE; // Tell mongoose to keep this connection open		
}

static int ev_handler(struct mg_connection *conn, enum mg_event ev) {
	switch (ev) {
		case MG_AUTH: return check_auth(conn);
		case MG_REQUEST: return serve_request(conn);
		case MG_POLL: return handle_poll(conn);
		default: return MG_FALSE;
	}
}


int main(void) {
	struct mg_server *server = mg_create_server(NULL, ev_handler);
	mg_set_option(server, "listening_port", "8085");
	mg_set_option(server, "document_root", ".");
	printf("Starting on port %s\n", mg_get_option(server, "listening_port"));

	
	rc = sqlite3_open("test", &db);
	if(rc){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return(1);
	}
	for (;;) {
		mg_poll_server(server, 1000);
	}

	mg_destroy_server(&server);
	return 0;
}

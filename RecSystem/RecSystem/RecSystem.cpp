// RecSystem.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include<iostream>
#include <stdio.h> 
#include<fstream>
#include<cmath>
#include<vector>
#include<algorithm>
#include<cstdlib>
#include <windows.h>

using namespace std;

struct rankingNode {
	int movieId;
	double rank;
};

struct analyseNode {
	int movieId;
	double rank;
	double calRank;
};

SYSTEMTIME sys;
vector<rankingNode>userMatrix[20000];
int** neighbor;
vector<analyseNode>preRank[20000];

int totalNum = 0;
int totalTest = 0;
int analyingNum = 0;
double totalRank = 0;
double totalRankAvg = 0;
int userNum = 0;
int movieNum = 0;
double** u2uSimMatrix;
vector<vector<int>>movieSawMatrix;
int rankingNums[20000];
double avgUserRanks[20000];

int rankedNums[630000];
int totalRanks[630000];
double avgMovieRanks[630000];

bool compByMovieId(rankingNode r1, rankingNode r2) {
	return r1.movieId < r2.movieId;
}

//读训练集,op为真表示读全部训练集，op为假表示读一部分训练集以做分析
void readfile(bool op) {
	totalRank = 0;
	totalRankAvg = 0;
	totalNum = 0;
	userNum = 0;
	movieNum = 0;

	ifstream in("train.txt");
	int user, movie, rank, rankingNum;
	double rankSum;
	char c;
	rankingNode tem;
	analyseNode tem2;
	if (op)                                   //读全部train.txt
		while (!in.eof()) {
			rankSum = 0;
			in >> user;
			c = in.get();
			if (c != '|')
				break;
			userNum++;
			in >> rankingNum;
			rankingNums[user] = rankingNum;
			for (int i = 0; i < rankingNum; i++) {
				in >> movie;
				in >> rank;
				totalRank += rank;
				totalRanks[movie] += rank;
				rankedNums[movie]++;
				totalNum++;
				if (movie > movieNum)
					movieNum = movie;
				rankSum += rank;
				tem.movieId = movie;
				tem.rank = rank;
				userMatrix[user].push_back(tem);
			}
			avgUserRanks[user] = rankSum / rankingNum;
			in.get();
			in.get();
		}
	else                                    //读部分train.txt
		while (!in.eof()) {
			rankSum = 0;
			in >> user;
			c = in.get();
			if (c != '|')
				break;
			userNum++;
			in >> rankingNum;
			rankingNums[user] = rankingNum;
			for (int i = 0; i < rankingNum; i++) {
				in >> movie;
				in >> rank;
				if (i % 100 ==0) {
					tem2.movieId = movie;
					tem2.rank = rank;
					preRank[user].push_back(tem2);
					analyingNum++;
				}
				else {
					totalRanks[movie] += rank;
					rankedNums[movie]++;
					totalRank += rank;
					totalNum++;
					if (movie > movieNum)
						movieNum = movie;
					rankSum += rank;
					tem.movieId = movie;
					tem.rank = rank;
					userMatrix[user].push_back(tem);
				}
			}
			avgUserRanks[user] = rankSum / rankingNum;
			in.get();
			in.get();
		}
	totalRankAvg = totalRank / totalNum;
	for (int i = 0; i < movieNum; i++) {
		if (rankedNums[i] != 0)
			avgMovieRanks[i] = totalRanks[i] / rankedNums[i];
	}
	for (int i = 0; i < movieNum; i++) {
		if (rankedNums[i] == 0)
			avgMovieRanks[i] = totalRankAvg;
	}
	cout << "打分人次：" << totalNum << endl;
	cout << "用户数：" << userNum << endl;
	cout << "电影数：" << movieNum << endl;
	cout << "所有电影均分：" << totalRankAvg << endl << endl;
}


//计算pearson相似度
double getSimilarity(int user1, int user2) {
	double det1 = 0, det2 = 0, cov = 0;
	double squ1 = 0, squ2 = 0;
	int i = 0, j = 0;
	while (i < userMatrix[user1].size() && j < userMatrix[user2].size()) {
		if (userMatrix[user1][i].movieId == userMatrix[user2][j].movieId) {
			int movie = userMatrix[user1][i].movieId;
			det1 = userMatrix[user1][i].rank - avgUserRanks[user1];
			det2 = userMatrix[user2][j].rank - avgUserRanks[user2];
			cov += det1*det2;
			squ1 += det1*det1;
			squ2 += det2*det2;
			i++;
			j++;
		}
		else
			if (userMatrix[user1][i].movieId < userMatrix[user2][j].movieId)
				i++;
			else
				j++;
	}
	if (squ1 > 0.000001&&squ2>0.000001) {
		return cov / (sqrt(squ1*squ2));
	}
	else
		return 0.0;
}

//生成相似度矩阵
void genu2uSimMatrix() {
	u2uSimMatrix = new double*[userNum];
	for (int i = 0; i < userNum; i++)
		sort(userMatrix[i].begin(), userMatrix[i].end(), compByMovieId);

	double xxx = getSimilarity(18652, 18652);

	for (int i = 0; i < userNum; i++)
		u2uSimMatrix[i] = new double[userNum];

	for (int i = 0; i < userNum; i++) {
		for (int j = i; j < userNum; j++) {
			u2uSimMatrix[i][j] = getSimilarity(i, j);
			u2uSimMatrix[j][i] = u2uSimMatrix[i][j];
		}
	}
}

//计算邻居
void getNeighbors(double param, int k) {
	neighbor = new int*[userNum];
	for (int i = 0; i<userNum; i++)
		neighbor[i] = new int[k];
	int tem = -1;
	double minsim = 1;
	int isolatedNum = 0;
	for (int i = 0; i < userNum; i++) {
		int curr = 0;
		for (int j = 0; j < userNum; j++) {
			if (abs(u2uSimMatrix[i][j] )> param) {
				if (curr< k) {
					neighbor[i][curr] = j;
					curr++;
				}
				else {                              //替换k个候选邻居中最远的那个
					minsim = 1;
					int nid;
					for (int a = 0; a< k; a++) {
						nid = neighbor[i][a];
						if (abs(u2uSimMatrix[i][nid])< minsim) {
							tem = a;
							minsim = abs(u2uSimMatrix[i][nid]);
						}
					}
					if (abs(u2uSimMatrix[i][j] )> minsim) {
						neighbor[i][tem] = j;
						minsim = 1;
					}
				}
			}
		}
		if (curr<k) {
			isolatedNum++;
			for (int t = curr; t < k; t++) {          //邻居数不够用自身填充
				neighbor[i][t] = i;
			}
		}
	}
	cout << "有" << isolatedNum << "名用户的达到阈值的邻居数不够\n\n";
}
//读测试集
void readTestFile(int k) {

	ifstream in("test.txt");
	int user, movie, rankingNum;

	char c;
	analyseNode tem;
	while (!in.eof()) {
		in >> user;
		c = in.get();
		if (c != '|')
			break;
		in >> rankingNum;
		for (int i = 0; i < rankingNum; i++) {
			in >> movie;
			tem.movieId = movie;
			preRank[user].push_back(tem);
			totalTest++;
		}
		in.get();
	}
	cout << "共有" << totalTest << "条待预测记录\n\n";
}


//预测打分
void predict(int k) {
	for (int i = 0; i < userNum; i++) {
		double bxi = 0;
		double bxj = 0;
		double rxj = 0;
		double powerOffset = 0;                          //加权偏移量之和
		double simSum = 0;									//相似度之和
		double grade = 0;
		for (int j = 0; j <preRank[i].size(); j++) {
			int movid = preRank[i][j].movieId;
			bxi = avgUserRanks[i];
			for (int t = 0; t < k; t++) {  //t对所有邻居遍历
				int neigid = neighbor[i][t];
				for (int p = 0; p < userMatrix[neigid].size(); p++)       //p对某个邻居看过的电影中遍历
					if (userMatrix[neigid][p].movieId == movid) {
						rxj = userMatrix[neigid][p].rank;
						bxj = avgUserRanks[neigid] ;
						double sim = u2uSimMatrix[i][neigid];
						powerOffset += sim *(rxj-bxj);
						simSum += abs(sim);
						break;
					}
			}
			if (abs(simSum) > 0.000001)
				grade =bxi+ powerOffset / simSum;
			else
				grade=bxi;	
				
				//没有邻居看过待预测的电影
			if (grade <= 0)                                      //化为0-100之间10的倍数
				grade = 0;
			else {
				if (grade >= 100)
					grade = 100;
				else {
					int a = grade / 10;
					double b = grade - a * 10;
					if (b < 5)
						grade = a * 10;
					else
						grade = a * 10 + 10;
				}
			}
			preRank[i][j].calRank = grade;

		}
	}
}

void writeResult() {
	ofstream o("result.txt");
	for (int i = 0; i < userNum; i++) {
		o << i << '|' << preRank[i].size() << endl;
		for (int j = 0; j <preRank[i].size(); j++) {
			o << preRank[i][j].movieId << "  " << preRank[i][j].calRank << "  " << endl;
		}
	}
}


//误差分析,取一部分训练集训练，剩余的训练集来做预测
void analyse(double param, int k) {
	
	for (int i = 0; i < userNum; i++)
		userMatrix[i].clear();
	for (int i = 0; i < userNum; i++)
		preRank[i].clear();
	readfile(false);
	genu2uSimMatrix();
	getNeighbors(param, k);
	predict(k);
	GetLocalTime(&sys);
	printf("%02d:%02d:%02d \n", sys.wHour, sys.wMinute, sys.wSecond);
	cout << "正在计算rmse...\n";
	int ann = 0;
	double se = 0;
	double rmse = 0;
	for (int i = 0; i < userNum; i++) {
		for (int j = 0; j < preRank[i].size(); j++) {
			ann++;
			se += (preRank[i][j].calRank - preRank[i][j].rank)*(preRank[i][j].calRank - preRank[i][j].rank);
		}
	}
	rmse = sqrt(se / ann);
	cout << "观测" << ann << "次\n";
	cout << "均方根误差为：" << rmse << endl;
}


int main() {
	int k =1000;
	double threshold = 0.5;
	
	GetLocalTime(&sys);
	printf("%02d:%02d:%02d \n", sys.wHour, sys.wMinute, sys.wSecond);
	cout << "正在读训练集...\n";
	readfile(true);

	GetLocalTime(&sys);
	printf("%02d:%02d:%02d \n", sys.wHour, sys.wMinute, sys.wSecond);
	cout << "正在计算相似度矩阵...\n\n";
	genu2uSimMatrix();

	GetLocalTime(&sys);
	printf("%02d:%02d:%02d \n", sys.wHour, sys.wMinute, sys.wSecond);
	cout << "正在计算最近邻矩阵...\n";
	getNeighbors(threshold, k);

	GetLocalTime(&sys);
	printf("%02d:%02d:%02d \n", sys.wHour, sys.wMinute, sys.wSecond);
	cout << "正在读测试集...\n\n";
	readTestFile(k);

	GetLocalTime(&sys);
	printf("%02d:%02d:%02d \n", sys.wHour, sys.wMinute, sys.wSecond);
	cout << "正在预测...\n\n";
	predict(k);

	GetLocalTime(&sys);
	printf("%02d:%02d:%02d \n", sys.wHour, sys.wMinute, sys.wSecond);
	cout << "预测完毕!!!\n\n";
	writeResult();
	
	GetLocalTime(&sys);
	printf("%02d:%02d:%02d \n", sys.wHour, sys.wMinute, sys.wSecond);
	cout << "开始误差分析，取部分训练集训练，剩余训练集来做预测\n";
	
	analyse(threshold, k);

	GetLocalTime(&sys);
	printf("\n%02d:%02d:%02d \n", sys.wHour, sys.wMinute, sys.wSecond);
	cout << "实验结束！！！\n";
}


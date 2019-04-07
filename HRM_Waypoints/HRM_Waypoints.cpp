// HRM_Waypoints.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include "pch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <queue>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <set>
#include <algorithm>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

#define HMS_INV -1000

struct waypoint
{
	std::string name = "";
	double longitude = HMS_INV;
	double latitude = HMS_INV;
	double longitude_head = HMS_INV;
	double latitude_head = HMS_INV;
	double longitude_head2 = HMS_INV;
	double latitude_head2 = HMS_INV;
};

std::vector<waypoint *> waypoint_vector;

double m_per_lat = HMS_INV;
double m_per_long = HMS_INV;
int min_dist = 100;
int max_waypoints = 100;
int w_index = 1;

inline bool exists_test(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

double calc_distance_m(double lat1, double long1, double lat2, double long2)
{
	lat1 = lat1 * M_PI / 180;
	long1 = long1 * M_PI / 180;
	lat2 = lat2 * M_PI / 180;
	long2 = long2 * M_PI / 180;

	double rEarth = 6372797;

	double dlat = lat2 - lat1;
	double dlong = long2 - long1;

	double x1 = sin(dlat / 2);
	double x2 = cos(lat1);
	double x3 = cos(lat2);
	double x4 = sin(dlong / 2);

	double x5 = x1 * x1;
	double x6 = x2 * x3 * x4 * x4;

	double temp1 = x5 + x6;

	double y1 = sqrt(temp1);
	double y2 = sqrt(1.0 - temp1);

	double temp2 = 2 * atan2(y1, y2);

	double range_m = temp2 * rEarth;

	return range_m;
}


void check_waypoint(double lat1, double long1, double lat2, double long2, int sub_type)
{
	if (lat1 == HMS_INV) return;
	if (lat2 == HMS_INV) return;
	if (long1 == HMS_INV) return;
	if (long2 == HMS_INV) return;

	if ((m_per_lat == HMS_INV) || (m_per_long == HMS_INV))
	{
		m_per_lat = abs(calc_distance_m(lat1, long1, lat1 + 1.0, long1));
		m_per_long = abs(calc_distance_m(lat1, long1, lat1, long1 + 1.0));
	}

	//if ((sub_type != 50) && (sub_type != 40) && (sub_type != 30) && (sub_type != 20)) return;
	if ((sub_type > 40) || (sub_type <= 20)) return;

	double delta_lat = abs(lat1 - lat2) * m_per_lat;
	double delta_long = abs(long1 - long2) * m_per_long;

	if ((delta_lat > min_dist) || (delta_long > min_dist))
	{
		waypoint *p_way = new waypoint();

		p_way->name = "WP_" + std::to_string(sub_type) + "_" + std::to_string(w_index++);
		p_way->latitude = ((lat1 + lat2) / 2.0);
		
		p_way->longitude = ((long1 + long2) / 2.0);
		
		p_way->latitude_head = lat1;
		p_way->longitude_head = long1;
		p_way->latitude_head2 = lat2;
		p_way->longitude_head2 = long2;

		waypoint_vector.push_back(p_way);
	}
}

int pnpoly(int nvert, float *vertx, float *verty, float testx, float testy)
{
	int i, j, c = 0;
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
			c = !c;
	}
	return c;
}

void CreateWaypointFile(std::string dsf_filename, std::string waypoint_filename)
{
	std::ifstream dsf_file(dsf_filename);
	std::string line_string;

	waypoint_vector.clear();

	double long_1 = HMS_INV;
	double lat_1 = HMS_INV;;
	double long_2 = HMS_INV;;
	double lat_2 = HMS_INV;;

	while (std::getline(dsf_file, line_string))
	{
		std::stringstream line_stream(line_string);

		std::string item_1 = "";
		int type = -1;
		int sub_type = -1;
		int node_id = -1;



		line_stream >> item_1;

		if (item_1.compare("BEGIN_SEGMENT") == 0)
		{
			line_stream >> type;
			line_stream >> sub_type;
			line_stream >> node_id;
			line_stream >> long_1;
			line_stream >> lat_1;


			bool end_segment = false;

			if (sub_type > 0)
			{
				while ((std::getline(dsf_file, line_string)) && (end_segment == false))
				{
					std::stringstream line_stream_2(line_string);

					std::string item_2 = "";
					line_stream_2 >> item_2;

					if (item_2.compare("SHAPE_POINT") == 0)
					{
						line_stream_2 >> long_2;
						line_stream_2 >> lat_2;


						check_waypoint(lat_1, long_1, lat_2, long_2, sub_type);

						lat_1 = lat_2;
						long_1 = long_2;

					}
					else if (item_2.compare("END_SEGMENT") == 0)
					{
						line_stream_2 >> node_id;

						line_stream_2 >> long_2;
						line_stream_2 >> lat_2;


						check_waypoint(lat_1, long_1, lat_2, long_2, sub_type);
						end_segment = true;

					}
					else
					{
						end_segment = true;
					}

				}
			}

		}


	}

	dsf_file.close();

	if (waypoint_vector.size() > 0)
	{

		std::ofstream fms_file;
		fms_file.open(waypoint_filename);

		if (fms_file.is_open())
		{
			fms_file << "I" << std::endl;
			fms_file << "1100 Version" << std::endl;
			fms_file << "CYCLE " << 1809 << std::endl;
			fms_file << "DEP " << std::endl;
			fms_file << "DES " << std::endl;
			fms_file << "NUMENR 3" << std::endl;

			fms_file.precision(9);

			int step_size = waypoint_vector.size() / max_waypoints;
			if (step_size <= 0) step_size = 1;

			for (int index = 0; index < waypoint_vector.size(); index += step_size)
			{
				waypoint *p_way = waypoint_vector[index];
				fms_file << "28 " << p_way->name << " DEP 25000.000000 " << p_way->latitude << " " << p_way->longitude << std::endl;
				p_way->name += "_HEAD";
				fms_file << "28 " << p_way->name << " DEP 25000.000000 " << p_way->latitude_head << " " << p_way->longitude_head << std::endl;
				//p_way->name += "_2";
				//fms_file << "28 " << p_way->name << " DEP 25000.000000 " << p_way->latitude_head2 << " " << p_way->longitude_head2 << std::endl;
			}

			fms_file.close();
		}
	}
}

int main(int argc, char *argv[])
{
	/*if (argc > 3)
	{
		std::cout << "Too many parameters!\n";
		std::cout << "Usage: HRM_Waypoints <uncompressed_dsf_file> <output_waypoint_file> \n";
		return 0;
	}
	else if (argc < 3)
	{
		std::cout << "Invalid parameters\n";
		std::cout << "Usage: HRM_Waypoints <uncompressed_dsf_file> <output_waypoint_file> \n";
		return 0;
	}

	std::string dsf_filename = argv[1];
	std::string waypoint_filename = argv[2];

	CreateWaypointFile(dsf_filename, waypoint_filename);*/

	for (int index_lat = -90; index_lat <= 90; index_lat++)
	{
		for (int index_long = -180; index_long <= 180; index_long++)
		{
			std::string dsf_filename = "";
			int short_lat = (index_lat / 10) * 10;
			int short_long = (index_long / 10) * 10;

			dsf_filename += short_lat < 0 ? "-" : "+";
			if ((short_lat < 10) && (short_lat > -10)) dsf_filename += "0";
			dsf_filename += std::to_string(abs(short_lat));

			dsf_filename += short_long < 0 ? "-" : "+";
			if ((short_long < 100) && (short_long > -100)) dsf_filename += "0";
			if ((short_long < 10) && (short_long > -10)) dsf_filename += "0";
			dsf_filename += std::to_string(abs(short_long));

			//dsf_filename += "\\";

			std::string waypoint_filename = "";

			waypoint_filename += index_lat < 0 ? "-" : "+";
			if ((index_lat < 10) && (index_lat > -10)) waypoint_filename += "0";
			waypoint_filename += std::to_string(abs(index_lat));

			waypoint_filename += index_long < 0 ? "-" : "+";
			if ((index_long < 100) && (index_long > -100)) waypoint_filename += "0";
			if ((index_long < 10) && (index_long > -10)) waypoint_filename += "0";
			waypoint_filename += std::to_string(abs(index_long));


			dsf_filename = "..\\scenery\\" + dsf_filename + "\\" + waypoint_filename;

			//waypoint_filename += ".fms";

			if (exists_test(dsf_filename + ".dsf") == true)
			{

				std::string command = "move " + dsf_filename + ".dsf " + dsf_filename + ".zip";
				system(command.c_str());

				command = "7z e " + dsf_filename + ".zip";
				system(command.c_str());

				command = "move " + dsf_filename + ".zip " + dsf_filename + ".dsf";
				system(command.c_str());

				//std::cout << "Evaluating: " << dsf_filename << std::endl;

				command = "DSFTool --dsf2text " + waypoint_filename + ".dsf " + waypoint_filename + ".txt";
				system(command.c_str());

				CreateWaypointFile(waypoint_filename + ".txt", waypoint_filename + ".fms");

				command = "del " + waypoint_filename + ".dsf";
				system(command.c_str());
				command = "del " + waypoint_filename + ".txt";
				system(command.c_str());
				command = "del *.raw";
				system(command.c_str());

			}
			else
			{
				std::cout << "Failed: " << dsf_filename << std::endl;
			}


		}
	}
	

	return 0;
}

// Programm ausführen: STRG+F5 oder "Debuggen" > Menü "Ohne Debuggen starten"
// Programm debuggen: F5 oder "Debuggen" > Menü "Debuggen starten"

// Tipps für den Einstieg: 
//   1. Verwenden Sie das Projektmappen-Explorer-Fenster zum Hinzufügen/Verwalten von Dateien.
//   2. Verwenden Sie das Team Explorer-Fenster zum Herstellen einer Verbindung mit der Quellcodeverwaltung.
//   3. Verwenden Sie das Ausgabefenster, um die Buildausgabe und andere Nachrichten anzuzeigen.
//   4. Verwenden Sie das Fenster "Fehlerliste", um Fehler anzuzeigen.
//   5. Wechseln Sie zu "Projekt" > "Neues Element hinzufügen", um neue Codedateien zu erstellen, bzw. zu "Projekt" > "Vorhandenes Element hinzufügen", um dem Projekt vorhandene Codedateien hinzuzufügen.
//   6. Um dieses Projekt später erneut zu öffnen, wechseln Sie zu "Datei" > "Öffnen" > "Projekt", und wählen Sie die SLN-Datei aus.

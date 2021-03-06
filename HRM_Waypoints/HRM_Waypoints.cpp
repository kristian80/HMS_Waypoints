// HRM_Waypoints.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//
#define _CRT_SECURE_NO_WARNINGS 1
#include "pch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <fstream>
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
#include <math.h>
#include <atlimage.h>
#include <filesystem>
//#include <boost/math/tools/barycentric_rational_interpolation.hpp>


//#include "fs/common/globereader.h"
//#include "geo/pos.h"

#define HRM_INV -1000
#define HRM_U_SECTOR 100

#define HRM_U_WAYPOINTS 400
#define HRM_S_WAYPOINTS 400


#define HRM_U_JUNCTION_DIST 5.0
#define HRM_U_URBAN_DIST 25.0
#define HRM_U_ANGLE_VAR 2.5
#define HRM_U_STREET_DIST 25.0

#define HRM_SAR_SECTOR_BIG 100
#define HRM_SAR_SECTOR_FINE 100
#define HRM_SAR_WAYPOINTS 200
#define HRM_SAR_URBAN_DIST 5 
#define HRM_SAR_WATER_DIST 3 

#define HRM_FLAT_SURFACE_DIST 200.0
#define HRM_FLAT_SURFACE_ANGLE 5

//const std::string scenery_dir = "..\\scenery\\";
const bool has_dem = true;
const int dem_width = 1201;
const int dem_height = 1201;

struct waypoint
{
	std::string name = "";
	double longitude = HRM_INV;
	double latitude = HRM_INV;
	double longitude_head = HRM_INV;
	double latitude_head = HRM_INV;
	double longitude_head2 = HRM_INV;
	double latitude_head2 = HRM_INV;
};

struct point
{
	double latitutde = HRM_INV;
	double longitude = HRM_INV;
};

struct street_section
{
	double latitutde1 = HRM_INV;
	double longitude1 = HRM_INV;
	double latitutde2 = HRM_INV;
	double longitude2 = HRM_INV;
};

struct street_junction
{
	double latitutdec = HRM_INV;
	double longitudec = HRM_INV;
	double latitutde2 = HRM_INV;
	double longitude2 = HRM_INV;
	double latitutde3 = HRM_INV;
	double longitude3 = HRM_INV;
};

struct urban_field
{
	std::vector<point> urban_polygon_points;
	std::vector<street_section> urban_street_sections;
	std::vector<street_junction> urban_street_junctions;
};



std::vector<waypoint *> waypoint_vector;

double m_per_lat = HRM_INV;
double m_per_long = HRM_INV;
double lat_per_m = HRM_INV;
double long_per_m = HRM_INV;

int min_dist = 100;
int max_waypoints = 100;
int w_index = 1;
//atools::fs::common::GlobeReader *globeReader = nullptr;

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
	if (lat1 == HRM_INV) return;
	if (lat2 == HRM_INV) return;
	if (long1 == HRM_INV) return;
	if (long2 == HRM_INV) return;

	if ((m_per_lat == HRM_INV) || (m_per_long == HRM_INV))
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



void CreateStreetWaypointFile(std::string dsf_filename, std::string waypoint_filename)
{
	std::ifstream dsf_file(dsf_filename);
	std::string line_string;
	for (auto wp : waypoint_vector)
		delete wp;
	waypoint_vector.clear();
	m_per_lat = HRM_INV;
	m_per_long = HRM_INV;
	min_dist = 100;
	max_waypoints = HRM_S_WAYPOINTS;
	w_index = 1;

	double long_1 = HRM_INV;
	double lat_1 = HRM_INV;;
	double long_2 = HRM_INV;;
	double lat_2 = HRM_INV;;

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

inline int GetUrbanSegment(double latitude, double longitude)
{


	double delta_lat = abs(latitude - ((int)latitude));
	double delta_long = abs(longitude - ((int)longitude));

	int index_lat = (int) (min(delta_lat / (1.0 / ((double)HRM_U_SECTOR)), ((double) HRM_U_SECTOR) - 1.0));
	int index_long = (int) (min(delta_long / (1.0 / ((double)HRM_U_SECTOR)), ((double)HRM_U_SECTOR) - 1.0));

	return (index_lat * HRM_U_SECTOR) + index_long;
}

void CreateUrbanWaypointFile(std::string dsf_filename, std::string waypoint_filename)
{
	std::ifstream dsf_file(dsf_filename);
	std::string line_string;
	for (auto wp : waypoint_vector)
		delete wp;
	waypoint_vector.clear();
	m_per_lat = HRM_INV;
	m_per_long = HRM_INV;
	min_dist = 100;
	max_waypoints = HRM_U_WAYPOINTS;
	w_index = 1;

	int poly_count = 0;
	int forest_count = 0;

	double long_1 = HRM_INV;
	double lat_1 = HRM_INV;
	double long_2 = HRM_INV;
	double lat_2 = HRM_INV;

	urban_field *p_urban;

	p_urban = new urban_field[HRM_U_SECTOR * HRM_U_SECTOR];


	while (std::getline(dsf_file, line_string))
	{
		std::stringstream line_stream(line_string);

		std::string item_1 = "";
		std::string item_2 = "";
		
		line_stream >> item_1;

		if (item_1.compare("POLYGON_DEF") == 0)
		{
			poly_count++;
			line_stream >> item_2;

			// we expect forests to come before everything else
			if (item_2.find(".for") != std::string::npos)
			{
				forest_count = poly_count;
			}
		}
		else if (item_1.compare("BEGIN_POLYGON") == 0)
		{
			bool end_segment = false;

			int poly_type = 0;
			line_stream >> poly_type;

			if (poly_type > forest_count)
			{
				while ((std::getline(dsf_file, line_string)) && (end_segment == false))
				{
					std::stringstream line_stream_2(line_string);

					std::string item_1 = "";
					line_stream_2 >> item_1;

					if (item_1.compare("BEGIN_WINDING") == 0)
					{
						// Do Nothing
					}
					else if (item_1.compare("POLYGON_POINT") == 0)
					{
						long_1 = HRM_INV;
						lat_1 = HRM_INV;

						line_stream_2 >> long_1;
						line_stream_2 >> lat_1;

						if ((lat_1 != HRM_INV) && (long_1 != HRM_INV))
						{
							if ((m_per_lat == HRM_INV) || (m_per_long == HRM_INV))
							{
								m_per_lat = abs(calc_distance_m(lat_1, long_1, lat_1 + 1.0, long_1));
								m_per_long = abs(calc_distance_m(lat_1, long_1, lat_1, long_1 + 1.0));
							}


							int segment = GetUrbanSegment(lat_1, long_1);

							point p;
							p.latitutde = lat_1;
							p.longitude = long_1;

							p_urban[segment].urban_polygon_points.push_back(p);
						}
					}
					else
					{
						end_segment = true;
					}

				}
			}
		}

		else if (item_1.compare("BEGIN_SEGMENT") == 0)
		{
			
			int type = -1;
			int sub_type = -1;
			int node_id = -1;

			std::vector<street_section> current_street_sections;
			
			line_stream >> type;
			line_stream >> sub_type;
			line_stream >> node_id;
			line_stream >> long_1;
			line_stream >> lat_1;


			bool end_segment = false;

			if ((sub_type > 30) && (sub_type < 80))
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

						if ((abs((lat_1 - lat_2) * m_per_lat) > HRM_U_STREET_DIST) || (abs((long_1 - long_2) * m_per_long) > HRM_U_STREET_DIST))
						{
							street_section sec;
							sec.latitutde1 = lat_1;
							sec.longitude1 = long_1;
							sec.latitutde2 = lat_2;
							sec.longitude2 = long_2;
							current_street_sections.push_back(sec);

							sec.latitutde2 = lat_1;
							sec.longitude2 = long_1;
							sec.latitutde1 = lat_2;
							sec.longitude1 = long_2;
							current_street_sections.push_back(sec);
						}


		

						lat_1 = lat_2;
						long_1 = long_2;

					}
					else if (item_2.compare("END_SEGMENT") == 0)
					{
						line_stream_2 >> node_id;

						line_stream_2 >> long_2;
						line_stream_2 >> lat_2;

						if ((abs((lat_1 - lat_2) * m_per_lat) > HRM_U_STREET_DIST) || (abs((long_1 - long_2) * m_per_long) > HRM_U_STREET_DIST))
						{

							street_section sec;
							sec.latitutde1 = lat_1;
							sec.longitude1 = long_1;
							sec.latitutde2 = lat_2;
							sec.longitude2 = long_2;
							current_street_sections.push_back(sec);

							sec.latitutde2 = lat_1;
							sec.longitude2 = long_1;
							sec.latitutde1 = lat_2;
							sec.longitude1 = long_2;
							current_street_sections.push_back(sec);
						}

						for (auto cs1 : current_street_sections)
						{
							int segment = GetUrbanSegment(cs1.latitutde1, cs1.longitude1);

							for (auto cs2 : p_urban[segment].urban_street_sections)
							{
								double delta_lat_m = (cs1.latitutde1 - cs2.latitutde1) * m_per_lat;
								double delta_long_m = (cs1.longitude1 - cs2.longitude1) * m_per_long;

								if ((abs(delta_lat_m) < HRM_U_JUNCTION_DIST) && (abs(delta_long_m) < HRM_U_JUNCTION_DIST))
								{
									double angle1 = atan2(cs1.latitutde1 - cs1.latitutde2, cs1.longitude1 - cs1.longitude2) * 180 / M_PI;
									double angle2 = atan2(cs2.latitutde1 - cs2.latitutde2, cs2.longitude1 - cs2.longitude2) * 180 / M_PI;

									if (angle1 < 0) angle1 += 360;
									if (angle2 < 0) angle2 += 360;

									double angle = angle1 - angle2;
									if (angle < 0) angle += 360;

									if (abs(angle - 90.0) <= HRM_U_ANGLE_VAR)
									{
										street_junction sj;

										sj.latitutdec = cs1.latitutde1;
										sj.longitudec = cs1.longitude1;

										sj.latitutde2 = cs1.latitutde2;
										sj.longitude2 = cs1.longitude2;

										sj.latitutde3 = cs2.latitutde2;
										sj.longitude3 = cs2.longitude2;

										p_urban[segment].urban_street_junctions.push_back(sj);
									}
									else if (abs(angle - 270.0) <= HRM_U_ANGLE_VAR)
									{
										street_junction sj;
										sj.latitutdec = cs1.latitutde1;
										sj.longitudec = cs1.longitude1;

										sj.latitutde3 = cs1.latitutde2;
										sj.longitude3 = cs1.longitude2;

										sj.latitutde2 = cs2.latitutde2;
										sj.longitude2 = cs2.longitude2;

										p_urban[segment].urban_street_junctions.push_back(sj);
									}
								}

							}
						}

						for (auto cs1 : current_street_sections)
						{
							int segment = GetUrbanSegment(cs1.latitutde1, cs1.longitude1);

							p_urban[segment].urban_street_sections.push_back(cs1);
						}

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

	// Get Junctions that are urban
	std::vector<street_junction> considered_street_junctions;

	for (int segment = 0; segment < (HRM_U_SECTOR * HRM_U_SECTOR); segment++)
	{
		for (auto junction : p_urban[segment].urban_street_junctions)
		{
			bool is_urban = false;
			for (auto poly : p_urban[segment].urban_polygon_points)
			{
				double delta_lat_m = (junction.latitutdec - poly.latitutde) * m_per_lat;
				double delta_long_m = (junction.longitudec - poly.longitude) * m_per_long;

				if ((abs(delta_lat_m) < HRM_U_URBAN_DIST) && (abs(delta_long_m) < HRM_U_URBAN_DIST))
				{
					is_urban = true;
					break;
				}
			}

			if (is_urban == true)
			{
				considered_street_junctions.push_back(junction);
			}
		}
	}


	

	if (considered_street_junctions.size() > 0)
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

			int step_size = considered_street_junctions.size() / max_waypoints;
			if (step_size <= 0) step_size = 1;

			for (int index = 0; index < considered_street_junctions.size(); index += step_size)
			{
				street_junction &p_junction = considered_street_junctions[index];
				fms_file << "28 " << "J" << index << " DEP 25000.000000 " << p_junction.latitutdec << " " << p_junction.longitudec << std::endl;
				fms_file << "28 " << "J" << index << "HEAD" << " DEP 25000.000000 " << p_junction.latitutde2 << " " << p_junction.longitude2 << std::endl;
				
			}

			fms_file.close();
		}
	}

	delete [] p_urban;


}

struct sar_field_big
{
	bool has_terrain = false;
	bool is_usable = true;

	double latitude = HRM_INV;
	double logitude = HRM_INV;

	float elevation = 0;
};

struct forest_winding
{
	std::vector<point> forest_polygon_points;
};

struct forest_polygon
{
	std::vector<forest_winding> forest_windings;
};



sar_field_big *ReadElevation(std::string file_name, int width, int height, int bpp, float scale, int offset);
bool getFlatPoint(std::vector<forest_polygon> &forest_vector, sar_field_big *p_sar, int width, int height, double lat_origin, double long_origin, int act_x, int act_y, int delta_x, int delta_y, waypoint &w_out, int index = 0);

inline int GetSARSegment(double latitude, double longitude)
{
	double delta_lat = latitude - ((int)latitude);
	double delta_long = longitude - ((int)longitude);

	int index_lat = (int)(min(delta_lat / (1.0 / ((double)HRM_SAR_SECTOR_BIG)), ((double)HRM_SAR_SECTOR_BIG) - 1.0));
	int index_long = (int)(min(delta_long / (1.0 / ((double)HRM_SAR_SECTOR_BIG)), ((double)HRM_SAR_SECTOR_BIG) - 1.0));

	return (index_lat * HRM_SAR_SECTOR_BIG) + index_long;
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

int pnpoly_cpp(std::vector<point> &points, double test_latitude, double test_longitude)
{
	int i, j, c = 0;
	for (i = 0, j = points.size() - 1; i < points.size(); j = i++) {
		if (((points[i].longitude > test_longitude) != (points[j].longitude > test_longitude)) &&
			(test_latitude < (points[j].latitutde - points[i].latitutde) * (test_longitude - points[i].longitude) / (points[j].longitude - points[i].longitude) + points[i].latitutde))
			c = !c;
	}
	return c;
}

void write_bmp_usable(sar_field_big*p_buffer, int width, int height);

inline float GetDistance(int x1, int y1, int x2, int y2)
{
	float delta_x = x1 - x2;
	float delta_y = y1 - y2;

	float distance = sqrt((delta_x * delta_x) + (delta_y * delta_y));
	return distance;
}

inline float ElevationInterpolation(int x, int y, int x1, int y1, float v1, int x2, int y2, float v2, int x3, int y3, float v3, int x4, int y4, float v4)
{
	float d1 = GetDistance(x, y, x1, y1);
	float d2 = GetDistance(x, y, x2, y2);
	float d3 = GetDistance(x, y, x3, y3);
	float d4 = GetDistance(x, y, x4, y4);

	// Invert
	d1 = 1 / (d1 + 0.0001);
	d2 = 1 / (d2 + 0.0001);
	d3 = 1 / (d3 + 0.0001);
	d4 = 1 / (d4 + 0.0001);

	// Normalize to sum 1
	float sum = d1 + d2 + d3 + d4;

	d1 = d1 / sum;
	d2 = d2 / sum;
	d3 = d3 / sum;
	d4 = d4 / sum;


	float value =	d1 * v1 +
					d2 * v2 +
					d3 * v3 +
					d4 * v4;

	return value;
	
}

void InterpolateElevation(sar_field_big*p_sar, int elev_width, int elev_height)
{
	for (int x = 0; x < elev_width; x++)
	{
		for (int y = 0; y < elev_width; y++)
		{
			if (p_sar[(y*elev_height) + x].elevation == 0)
			{
				bool abort = false;
				bool found = false;
				int xla = -1, yla = -1;
				int xra = -1, yra = -1;
				int xlb = -1, ylb = -1;
				int xrb = -1, yrb = -1;
				

				for (int step_size = 1; (step_size <= 100) && (abort == false) && (found == false); step_size++)
				{
					for (int point = 0; (point < (step_size * 2)) && (abort == false) && (found == false); point++)
					{
						int a = step_size;
						if (point > (step_size + 1))
							a = step_size - (point - (step_size + 1));

						int b = point > (step_size + 1) ? step_size + 1 : point;

						if ((xla == -1) && (yla == -1))
						{
							int x1 = x - a;
							int y1 = y - b;

							if ((x1 < 0) || (x1 >= elev_width) || (y1 < 0) || (y1 > elev_height))
							{
								abort = true;
							}
							else
							{
								if (p_sar[(y1*elev_height) + x1].elevation > 0)
								{
									xla = x1;
									yla = y1;
								}
							}
						}

						if ((xra == -1) && (yra == -1))
						{
							int x1 = x + b;
							int y1 = y - a;

							if ((x1 < 0) || (x1 >= elev_width) || (y1 < 0) || (y1 > elev_height))
							{
								abort = true;
							}
							else
							{
								if (p_sar[(y1*elev_height) + x1].elevation > 0)
								{
									xra = x1;
									yra = y1;
								}
							}
						}

						if ((xlb == -1) && (ylb == -1))
						{
							int x1 = x - b;
							int y1 = y + a;

							if ((x1 < 0) || (x1 >= elev_width) || (y1 < 0) || (y1 > elev_height))
							{
								abort = true;
							}
							else
							{
								if (p_sar[(y1*elev_height) + x1].elevation > 0)
								{
									xlb = x1;
									ylb = y1;
								}
							}
						}

						if ((xrb == -1) && (yrb == -1))
						{
							int x1 = x + a;
							int y1 = y - b;

							if ((x1 < 0) || (x1 >= elev_width) || (y1 < 0) || (y1 > elev_height))
							{
								abort = true;
							}
							else
							{
								if (p_sar[(y1*elev_height) + x1].elevation > 0)
								{
									xrb = x1;
									yrb = y1;
								}
							}
						}

						if ((xla != -1) && (yla != -1) &&
							(xra != -1) && (yra != -1) &&
							(xlb != -1) && (ylb != -1) &&
							(xrb != -1) && (yrb != -1))
						{
							found = true;

							

							
						}

					}

					
				}
				
				if (found == true)
				{
					int x_start = xla;
					int x_stop = xla;

					x_start = min(x_start, xra);
					x_start = min(x_start, xlb);
					x_start = min(x_start, xrb);

					x_stop = max(x_stop, xra);
					x_stop = max(x_stop, xlb);
					x_stop = max(x_stop, xrb);

					int y_start = yla;
					int y_stop = yla;

					y_start = min(y_start, yra);
					y_start = min(y_start, ylb);
					y_start = min(y_start, yrb);

					y_stop = max(y_stop, yra);
					y_stop = max(y_stop, ylb);
					y_stop = max(y_stop, yrb);

					float x_vektor[4] = { xla, xra, xlb, xrb };
					float y_vektor[4] = { yla, yra, ylb, yrb };

					for (int xv = x_start; xv < x_stop; xv++)
					{
						for (int yv = y_start; yv < y_stop; yv++)
						{
							if (p_sar[(yv * elev_height) + xv].elevation == 0)
							{
								if (pnpoly(4, x_vektor, y_vektor, xv, yv) > 0)
								{
									float elev = ElevationInterpolation(xv, yv, xla, yla, p_sar[(yla * elev_height) + xla].elevation,
										xra, yra, p_sar[(yra * elev_height) + xra].elevation,
										xlb, ylb, p_sar[(ylb * elev_height) + xlb].elevation,
										xrb, yrb, p_sar[(yrb * elev_height) + xrb].elevation);

									p_sar[(yv * elev_height) + xv].elevation = elev;
									p_sar[(yv * elev_height) + xv].is_usable = p_sar[(yla * elev_height) + xla].is_usable && p_sar[(yra * elev_height) + xra].is_usable && p_sar[(ylb * elev_height) + xlb].is_usable && p_sar[(yrb * elev_height) + xrb].is_usable;
								}
							}
						}
					}

				}
			}

		}
	}
}

void CreateSARWaypointFile(std::string dsf_filename, std::string waypoint_filename, double tile_lat, double tile_long)
{
	std::ifstream dsf_file(dsf_filename);
	std::string line_string;

	sar_field_big *p_sar = NULL;

	for (auto wp : waypoint_vector)
		delete wp;
	waypoint_vector.clear();
	m_per_lat = HRM_INV;
	m_per_long = HRM_INV;
	min_dist = 100;
	max_waypoints = HRM_SAR_WAYPOINTS;
	w_index = 1;

	int poly_count = 0;
	int forest_count = 0;
	int terrain_type = 0;

	double long_1 = HRM_INV;
	double lat_1 = HRM_INV;
	double long_2 = HRM_INV;
	double lat_2 = HRM_INV;

	bool is_water = false;

	
	std::vector<point> water_points;
	

	std::vector<forest_polygon> forest_vector;
	std::vector<forest_polygon> urban_vector;
	std::vector<waypoint> considered_points;

	std::vector<int> water_defs;

	int elevation_pos = 0;
	bool elevation_pos_found = false;

	//bool elevation_file_found = false;
	int elevation_file_count = 0;

	int elev_width = 1201;
	int elev_height = 1201;
	int elev_bpp = 2;
	float elev_scale = 1.0f;
	float elev_offset = 0;
	int elev_version = 0;
	int terrain_def_count = 0;

	std::string elev_filename = "";


	while (std::getline(dsf_file, line_string))
	{
		std::stringstream line_stream(line_string);

		std::string item_1 = "";
		std::string item_2 = "";

		line_stream >> item_1;

		if (item_1.compare("TERRAIN_DEF") == 0)
		{
			line_stream >> item_2;
			//if ((item_2.find("Water") != std::string::npos) || (item_2.find("water") != std::string::npos))
			//	water_defs.push_back(terrain_def_count);

			terrain_def_count++;
		}

		else if (item_1.compare("RASTER_DEF") == 0) 
		{
			line_stream >> item_2;

			if (item_2.compare("elevation") == 0)
			{
				elevation_pos_found = true;
			}
			else
			{
				if (elevation_pos_found == false) elevation_pos++;
			}
		}
		else if (item_1.compare("RASTER_DATA") == 0) 
		{
			if (elevation_file_count == elevation_pos)
			{
				line_stream >> item_2; // version
				line_stream >> item_2; // bpp
				sscanf_s(item_2.c_str(), "bpp=%i", &elev_bpp);

				line_stream >> item_2; // flags

				line_stream >> item_2; // width
				sscanf_s(item_2.c_str(), "width=%i", &elev_width);

				line_stream >> item_2; // height
				sscanf_s(item_2.c_str(), "height=%i", &elev_height);

				line_stream >> item_2; // scale
				sscanf_s(item_2.c_str(), "scale=%f", &elev_scale);

				line_stream >> item_2; // offset
				sscanf_s(item_2.c_str(), "offset=%f", &elev_offset);

				line_stream >> elev_filename;

			}
			elevation_file_count++;
		}



		else if (item_1.compare("POLYGON_DEF") == 0)
		{
			is_water = false;
			poly_count++;
			line_stream >> item_2;

			// we expect forests to come before everything else
			if (item_2.find(".for") != std::string::npos)
			{
				forest_count = poly_count;
			}
		}
		else if (item_1.compare("BEGIN_POLYGON") == 0)
		{
			bool end_segment = false;
			is_water = false;
			int winding = 0;

			int poly_type = 0;
			line_stream >> poly_type;

			forest_polygon current_forest;
			forest_winding current_winding;

			while ((std::getline(dsf_file, line_string)) && (end_segment == false))
			{
				std::stringstream line_stream_2(line_string);

				std::string item_1 = "";
				line_stream_2 >> item_1;

				

				if (item_1.compare("BEGIN_WINDING") == 0)
				{
					winding++;
					if (winding == 1)
						current_winding.forest_polygon_points.clear();
				}

				else if (item_1.compare("END_WINDING") == 0)
				{
					if (winding == 1)
						current_forest.forest_windings.push_back(current_winding);
				}
				else if (item_1.compare("POLYGON_POINT") == 0)
				{
					long_1 = HRM_INV;
					lat_1 = HRM_INV;

					line_stream_2 >> long_1;
					line_stream_2 >> lat_1;

					//if (poly_type <= forest_count)
					{

						//if (current_winding.forest_polygon_points.size() == 0)
						{
							point current_point;
							current_point.latitutde = lat_1;
							current_point.longitude = long_1;
							if (winding == 1)
								current_winding.forest_polygon_points.push_back(current_point);
						}
						/*else
						{
							//if ((current_winding.forest_polygon_points[current_winding.forest_polygon_points.size() - 1].latitutde != lat_1) ||
							//	(current_winding.forest_polygon_points[current_winding.forest_polygon_points.size() - 1].longitude != long_1))
							{
								point current_point;
								current_point.latitutde = lat_1;
								current_point.longitude = long_1;
								current_winding.forest_polygon_points.push_back(current_point);
							}
						}*/
					}
					// For Urban Polygon Points (far smaller than forests)
					/*else
					{
						int segment = GetSARSegment(lat_1, long_1);
						p_sar[segment].has_urban = true;
					}*/
				}
					
				else if(item_1.compare("END_POLYGON") == 0)
				{
					if (/*(poly_type <= forest_count) && */(current_forest.forest_windings.size() > 0))
					{
						forest_vector.push_back(current_forest);
					}
					else if (current_forest.forest_windings.size() > 0)
					{
						urban_vector.push_back(current_forest);
					}

					end_segment = true;
				}
				else
				{
					end_segment = true;
				}

			}
			
		}

		else if (item_1.compare("BEGIN_PATCH") == 0)
		{
			terrain_type = 0;

			line_stream >> terrain_type;

			if (terrain_type == 0)
			{
				is_water = true;
			}
			else
			{
				is_water = false;
				for (auto t_number : water_defs)
				{
					if (t_number == terrain_type) 
						is_water = true;
				}

				
			}


		}

		else if (item_1.compare("END_PATCH") == 0)
		{
			is_water = false;
		}

		else if (item_1.compare("PATCH_VERTEX") == 0)
		{
			if (elev_filename.size() > 0)
			{
				if (is_water == true)
				{
					point p;
					line_stream >> p.longitude;
					line_stream >> p.latitutde;

					water_points.push_back(p);
				}
			}
			else
			{
				if (p_sar == NULL)
				{
					p_sar = new sar_field_big[dem_height * dem_width];
					elev_height = dem_height;
					elev_width = dem_width;

					for (int index = 0; index < (elev_height * elev_width); index++)
					{
						p_sar[index].is_usable = false;
					}
				}
				else
				{
					double latitude;
					double longitude;
					float elevation;

					line_stream >> longitude;
					line_stream >> latitude;
					line_stream >> elevation;

					int x = min((int) ((longitude - tile_long) * ((double) (elev_width - 1))), elev_width -1);
					int y = min((int) ((latitude - tile_lat) * ((double)(elev_height - 1))), elev_height - 1);

					p_sar[(y*elev_height) + x].is_usable = !is_water;
					p_sar[(y*elev_height) + x].elevation = elevation;

				}
			}
		}

		else if (item_1.compare("BEGIN_SEGMENT") == 0)
		{

			int type = -1;
			int sub_type = -1;
			int node_id = -1;
			point p;

			line_stream >> type;
			line_stream >> sub_type;
			line_stream >> node_id;
			line_stream >> p.longitude;
			line_stream >> p.latitutde;

			water_points.push_back(p);
		}

		else if (item_1.compare("END_SEGMENT") == 0)
		{
			int node_id = -1;
			point p;

			line_stream >> node_id;

			line_stream >> p.longitude;
			line_stream >> p.latitutde;

			water_points.push_back(p);
		}
		else if (item_1.compare("SHAPE_POINT") == 0)
		{
			point p;
			line_stream >> p.longitude;
			line_stream >> p.latitutde;

			water_points.push_back(p);
		}

		/*else if (item_1.compare("PATCH_VERTEX") == 0)
		{
			lat_1 = HRM_INV;
			long_1 = HRM_INV;

			line_stream >> long_1;
			line_stream >> lat_1;

			double d1, d2, d3, d4;
			double steepness = 0;

			line_stream >> d1;
			line_stream >> d2;
			line_stream >> d3;
			line_stream >> d4;

			line_stream >> steepness;



			if ((long_1 > 0) && (lat_1 > 0))
			{

				int segment = GetSARSegment(lat_1, long_1);
				p_sar[segment].has_terrain = true;

				if ((steepness > 0) && (steepness < 1.0))
				{
					if (p_sar[segment].steepness > steepness)
					{
						p_sar[segment].steepness = steepness;
						p_sar[segment].latitude = lat_1;
						p_sar[segment].logitude = long_1;
					}
				}


			}


		}*/


	}

	dsf_file.close();

	double delta_lat = 1.0 / ((double) (elev_height-1));
	double delta_long = 1.0 / ((double) (elev_width-1));

	
	if (p_sar == NULL) p_sar = ReadElevation(elev_filename, elev_width, elev_height, elev_bpp, elev_scale, elev_offset);
	else InterpolateElevation(p_sar, elev_width, elev_height);

	

	for (auto p : water_points)
	{
		p.latitutde -= tile_lat;
		p.longitude -= tile_long;

		int x = min(p.longitude * (elev_width-1), elev_width-1);
		int y = min(p.latitutde * (elev_height-1), elev_height - 1);

		int x_start = max(0, x - HRM_SAR_WATER_DIST);
		int x_stop  = min(elev_width-1, x + HRM_SAR_WATER_DIST);

		int y_start = max(0, y - HRM_SAR_WATER_DIST);
		int y_stop  = min(elev_height - 1, y + HRM_SAR_WATER_DIST);

		for (x = x_start; x<= x_stop; x++)
			for (y = y_start; y <= y_stop; y++)
				p_sar[(y*elev_height) + x].is_usable = false;

	}

	for (auto poly_vector : urban_vector)
	{
		for (auto poly : poly_vector.forest_windings)
		{
			if (poly.forest_polygon_points.size() > 2)
			{
				point pmax = poly.forest_polygon_points[0];
				point pmin = poly.forest_polygon_points[0];

				for (auto p : poly.forest_polygon_points)
				{
					pmax.latitutde = max(pmax.latitutde, p.latitutde);
					pmax.longitude = max(pmax.longitude, p.longitude);
					pmin.latitutde = min(pmin.latitutde, p.latitutde);
					pmin.longitude = min(pmin.longitude, p.longitude);
				}

				int x_start = abs(pmin.longitude - tile_long) / delta_long;
				int x_stop = abs(pmax.longitude - tile_long) / delta_long;

				int y_start = abs(pmin.latitutde - tile_lat) / delta_lat;
				int y_stop = abs(pmax.latitutde - tile_lat) / delta_lat;

				x_start = max(x_start - HRM_SAR_URBAN_DIST, 0);
				y_start = max(y_start - HRM_SAR_URBAN_DIST, 0);

				x_stop = min(x_stop + HRM_SAR_URBAN_DIST, elev_width);
				y_stop = min(y_stop + HRM_SAR_URBAN_DIST, elev_height);

				for (int x = x_start; x < x_stop; x++)
				{
					for (int y = y_start; y < y_stop; y++)
					{
						p_sar[(y*elev_height) + x].is_usable = false;
					}

				}
			}
		}

	}

	for (auto windings : forest_vector)
	{
		
		// Get Rectangular Border
		for (auto current_winding : windings.forest_windings)
		//if (windings.forest_windings.size() > 0)
		{
			point pmax = current_winding.forest_polygon_points[0];
			point pmin = current_winding.forest_polygon_points[0];

			for (auto p : current_winding.forest_polygon_points)
			{
				pmax.latitutde = max(pmax.latitutde, p.latitutde);
				pmax.longitude = max(pmax.longitude, p.longitude);
				pmin.latitutde = min(pmin.latitutde, p.latitutde);
				pmin.longitude = min(pmin.longitude, p.longitude);
			}

			int x_start = abs(pmin.longitude - tile_long) / delta_long;
			int x_stop = abs(pmax.longitude - tile_long) / delta_long;

			int y_start = abs(pmin.latitutde - tile_lat) / delta_lat;
			int y_stop = abs(pmax.latitutde - tile_lat) / delta_lat;

			x_start = max(x_start - HRM_SAR_URBAN_DIST, 0);
			y_start = max(y_start - HRM_SAR_URBAN_DIST, 0);

			x_stop = min(x_stop + HRM_SAR_URBAN_DIST, elev_width);
			y_stop = min(y_stop + HRM_SAR_URBAN_DIST, elev_height);

			for (int x = x_start; x < x_stop; x++)
			{
				for (int y = y_start; y < y_stop; y++)
				{
					double f_lat = tile_lat + ((double) y) / ((double)(elev_height - 1));
					double f_long = tile_long + ((double)x) / ((double)(elev_width - 1));

					bool is_forest = false;

					// Add last point
					//current_winding.forest_polygon_points.push_back(current_winding.forest_polygon_points[0]);

					// Check if point is in forest
					if (pnpoly_cpp(current_winding.forest_polygon_points, f_lat, f_long) > 0) 
						is_forest = true;
						
					
					// Remove clearings
					/*if (is_forest == true)
					{
						for (int index = 1; index < windings.forest_windings.size(); index++)
						{
							if (pnpoly_cpp(current_winding.forest_polygon_points, f_lat, f_long) > 0)
								is_forest = false;
						}
					}*/

					if (is_forest == true)
						p_sar[(y*elev_height) + x].is_usable = false;

				}

			}

		}


		

	}
	//write_bmp_usable(p_sar, elev_width, elev_height);


	write_bmp_usable(p_sar, elev_width, elev_height);
	
	int delta_h = (elev_height - 1) / HRM_SAR_SECTOR_BIG;
	int delta_w = (elev_width - 1) / HRM_SAR_SECTOR_BIG;

	for (int x = 0; x < HRM_SAR_SECTOR_BIG; x++)
	{
		for (int y = 0; y < HRM_SAR_SECTOR_BIG; y++)
		{
			waypoint w_act;
			if (getFlatPoint(forest_vector, p_sar, elev_width, elev_height, tile_lat, tile_long, x*delta_w, y*delta_h, delta_w, delta_h, w_act))
			{
				
				considered_points.push_back(w_act);
			}


		}
	}

	

	/*for (int offset_lat = 0; offset_lat < HRM_SAR_SECTOR_BIG; offset_lat++)
	{
		for (int offset_long = 0; offset_long < HRM_SAR_SECTOR_BIG; offset_long++)
		{
			int segment = (offset_lat * HRM_SAR_SECTOR_BIG) + offset_long;

			if ((p_sar[segment].has_terrain == true) && (p_sar[segment].has_urban == false))
			{

				if ((p_sar[segment].latitude != HRM_INV) && (p_sar[segment].logitude != HRM_INV) && (p_sar[segment].steepness < 10000))
				{
					int result = 0;
					for (auto forest : forest_vector)
					{
						result += pnpoly_cpp(forest.forest_polygon_points, p_sar[segment].latitude, p_sar[segment].logitude);

					}

					if (result == 0)
					{
						point current_point;
						current_point.latitutde = p_sar[segment].latitude;
						current_point.longitude = p_sar[segment].logitude;
						considered_points.push_back(current_point);
					}
					else
					{
						result++;
					}


				}
			}
		}

	}*/




	/*for (int offset_lat = 0; offset_lat < HRM_SAR_SECTOR_BIG; offset_lat++)
	{
		for (int offset_long = 0; offset_long < HRM_SAR_SECTOR_BIG; offset_long++)
		{
			int segment = (offset_lat * HRM_SAR_SECTOR_BIG) + offset_long;

			if ((p_sar[segment].has_terrain == true) && (p_sar[segment].has_urban == false))
			{
				for (	double current_latitude = tile_lat + (1.0 / (2 * HRM_SAR_SECTOR_FINE)) + (((double)offset_lat) / HRM_SAR_SECTOR_BIG);
						current_latitude < (tile_lat + (((double)(offset_lat + 1) / HRM_SAR_SECTOR_BIG)));
						current_latitude += (1.0 / HRM_SAR_SECTOR_FINE))
				{
					for (	double current_longitude = tile_long + (1.0 / (2 * HRM_SAR_SECTOR_FINE)) + (((double)offset_long) / HRM_SAR_SECTOR_BIG);
							current_longitude < (tile_long + (((double) (offset_long + 1)) / HRM_SAR_SECTOR_BIG));
							current_longitude += (1.0 / HRM_SAR_SECTOR_FINE))
					{
						int result = 0;
						for (auto forest : forest_vector)
						{
							result += pnpoly_cpp(forest.forest_polygon_points, current_latitude, current_longitude);
							
						}

						if (result == 0)
						{
							point current_point;
							current_point.latitutde = current_latitude;
							current_point.longitude = current_longitude;
							considered_points.push_back(current_point);
						}
						else
						{
							result++;
						}


					}
				}
			}
		}

	}*/

	if (considered_points.size() > 0)
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

			int step_size = considered_points.size() / max_waypoints;
			if (step_size <= 0) step_size = 1;

			for (int index = 0; index < considered_points.size(); index += step_size)
			{
				waypoint &p = considered_points[index];
				fms_file << "28 " << p.name << index << " DEP 30000.000000 " << p.latitude << " " << p.longitude << std::endl;
				fms_file << "28 " << p.name << "HEAD_" << index << " DEP 30000.000000 " << p.latitude_head << " " << p.longitude_head << std::endl;
			}

			fms_file.close();
		}
	}

	if (p_sar != NULL) delete[] p_sar;


}

/*bool CheckForestFree(std::vector<forest_polygon> &forest_vector, double latitude, double longitude)
{
	int result = 0;
	for (auto forest : forest_vector)
	{
		result += pnpoly_cpp(forest.forest_polygon_points, latitude, longitude);
	}

	if (result == 0) return true;
	return false;
}*/


bool getFlatPoint(std::vector<forest_polygon> &forest_vector, sar_field_big *p_sar, int width, int height, double lat_origin, double long_origin, int act_x, int act_y, int delta_x, int delta_y, waypoint &w_out, int index)
{
	bool found = false;

	static float delta_lat = 0;
	static float delta_long = 0;
	static float m_dev = 0;

	if ((lat_per_m == HRM_INV) || (long_per_m == HRM_INV))
	{
		m_per_lat = abs(calc_distance_m(lat_origin, long_origin, lat_origin + 1.0, long_origin));
		m_per_long = abs(calc_distance_m(lat_origin, long_origin, lat_origin, long_origin + 1.0));
	}

	if (index == 0)
	{
		delta_lat = 2.0f / (float) (width-1);
		delta_long = 2.0f / (float) (height-1);
		float m_dev_lat = sin((HRM_FLAT_SURFACE_ANGLE * M_PI) / 180.0f) * m_per_lat * delta_lat;
		float m_dev_long = sin((HRM_FLAT_SURFACE_ANGLE * M_PI) / 180.0f) * m_per_long * delta_long;
		m_dev = min(m_dev_lat, m_dev_long);
	}


	for (int center_x = act_x + 1; (center_x < (act_x + delta_x)) && (found == false); center_x++)
	{
		for (int center_y = act_y + 1; (center_y < (act_y + delta_y)) && (found == false); center_y++)
		{

			//int center_x = act_x + (delta_x / 2);
			//int center_y = act_y + (delta_y / 2);

			double head_x = p_sar[(center_y*width) + center_x].elevation - p_sar[(center_y*width) + center_x + 1].elevation;
			double head_y = p_sar[(center_y*width) + center_x].elevation - p_sar[((center_y + 1)*width) + center_x].elevation;


			if ((center_x > 0) && (center_x < width) && (center_y > 0) && (center_y < height))
			{

				bool flat_surface = false;

				sar_field_big &sar_c = p_sar[(center_y*width) + center_x];
				sar_field_big *sar_around[8];



				sar_around[0] = &p_sar[((center_y - 1)*width) + (center_x - 1)];
				sar_around[1] = &p_sar[((center_y - 1)*width) + (center_x)];
				sar_around[2] = &p_sar[((center_y - 1)*width) + (center_x + 1)];

				sar_around[3] = &p_sar[((center_y)*width) + (center_x - 1)];
				sar_around[4] = &p_sar[((center_y)*width) + (center_x + 1)];

				sar_around[5] = &p_sar[((center_y + 1)*width) + (center_x - 1)];
				sar_around[6] = &p_sar[((center_y + 1)*width) + (center_x)];
				sar_around[7] = &p_sar[((center_y + 1)*width) + (center_x + 1)];


				float min_elevation = sar_c.elevation;
				float max_elevation = sar_c.elevation;



				for (int index = 0; index < 8; index++)
				{
					min_elevation = min(min_elevation, sar_around[index]->elevation);
					max_elevation = max(max_elevation, sar_around[index]->elevation);
				}

				float delta_elevation = max_elevation - min_elevation;

				if ((delta_elevation <= m_dev) && (delta_elevation > 0) && (sar_c.is_usable == true))
				{
					found = true;

					for (int index = 0; index < 8; index++)
					{
						if (sar_around[index]->is_usable == false)
							found = false;
					}
				}

				//index++;

				if (found == true)
				{

					double latitude = lat_origin + ((((double)center_y)/* + 0.2*/) / ((double)(height-1)));
					double longitude = long_origin + ((((double)center_x)/* + 0.2*/) / ((double)(width-1)));

					w_out.name = "P_" + std::to_string(((int)head_x)) + "_" + std::to_string(((int)head_y)) + "_" + std::to_string(((int)delta_elevation)) + "_";
					w_out.latitude = latitude;
					w_out.longitude = longitude;

					w_out.latitude_head = latitude + (head_y * 200.0 / m_per_lat);
					w_out.longitude_head = longitude + (head_x * 200.0 / m_per_long);

					/*std::ofstream fms_file;
					fms_file.open("test.fms");
					fms_file.precision(9);

					fms_file << "I" << std::endl;
					fms_file << "1100 Version" << std::endl;
					fms_file << "CYCLE " << 1809 << std::endl;
					fms_file << "DEP " << std::endl;
					fms_file << "DES " << std::endl;
					fms_file << "NUMENR 3" << std::endl;

					fms_file << "28 " << "J" << index << " DEP 25000.000000 " << latitude << " " << longitude << std::endl;
					fms_file.close();*/


				}
			}
		}
	}
	
	/*if ((++index < 3) && (found == false))
	{
		if (found == false) found = getFlatPoint(forest_vector, p_sar, width, height, lat_origin, long_origin, act_x, act_y, delta_x / 2, delta_y / 2, w_out, index);
		if (found == false) found = getFlatPoint(forest_vector, p_sar, width, height, lat_origin, long_origin, act_x + (delta_x / 2), act_y, delta_x / 2, delta_y / 2, w_out, index);
		if (found == false) found = getFlatPoint(forest_vector, p_sar, width, height, lat_origin, long_origin, act_x, act_y + (delta_y / 2), delta_x / 2, delta_y / 2, w_out, index);
		if (found == false) found = getFlatPoint(forest_vector, p_sar, width, height, lat_origin, long_origin, act_x + (delta_x / 2), act_y + (delta_y / 2), delta_x / 2, delta_y / 2, w_out, index);
	}*/

	return found;

}






int pnpoly_orig(int nvert, float *vertx, float *verty, float testx, float testy)
{
	int i, j, c = 0;
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
			c = !c;
	}
	return c;
}

sar_field_big *ReadElevation(std::string file_name, int width, int height, int bpp, float scale, int offset)
{
	sar_field_big *p_buffer = new sar_field_big[width * height];
	unsigned char *p_cbuffer = new unsigned char[width * height * bpp];

	memset(p_buffer, 0, width * height);

	FILE* infile;
		
	fopen_s(&infile,file_name.c_str(), "rb");

	fread(p_cbuffer, bpp, width * height, infile);

	fclose(infile);

	

	for (int h = 0; h < (height); h++)
		for (int w = 0; w < (width); w++)
		{
			//unsigned char byte;

			p_buffer[(h*width) + w].elevation = 0;
		
			for (int b_index = 0; b_index < bpp; b_index++)
			{
				

				//p_buffer[((elev_height - h - 1)*elev_width) + w] += p_cbuffer[(elev_bpp*((h*elev_width) + w)) + b_index]  << (8*b_index);
				p_buffer[(h*width) + w].elevation += p_cbuffer[(bpp*((h*width) + w)) + b_index] << (8 * b_index);
			}
			//p_buffer[(h*elev_width) + w] = (unsigned int) ((((float)p_buffer[index]) * elev_scale) + elev_offset);
		}

	delete[] p_cbuffer;

	return p_buffer;

}

struct BMPFileHeader {
	uint16_t file_type{ 0x4D42 };          // File type always BM which is 0x4D42
	uint32_t file_size{ 0 };               // Size of the file (in bytes)
	uint16_t reserved1{ 0 };               // Reserved, always 0
	uint16_t reserved2{ 0 };               // Reserved, always 0
	uint32_t offset_data{ 0 };             // Start position of pixel data (bytes from the beginning of the file)
	
};

struct BMPInfoHeader{
     uint32_t size{ 0 };                      // Size of this header (in bytes)
     int32_t width{ 0 };                      // elev_width of bitmap in pixels
     int32_t height{ 0 };                     // elev_width of bitmap in pixels
                                              //       (if positive, bottom-up, with origin in lower left corner)
                                              //       (if negative, top-down, with origin in upper left corner)
     uint16_t planes{ 1 };                    // No. of planes for the target device, this is always 1
     uint16_t bit_count{ 32 };                 // No. of bits per pixel
     uint32_t compression{ 3 };               // 0 or 3 - uncompressed. THIS PROGRAM CONSIDERS ONLY UNCOMPRESSED BMP images
     uint32_t size_image{ 0 };                // 0 - for uncompressed images
     int32_t x_pixels_per_meter{ 1024 };
     int32_t y_pixels_per_meter{ 1024 };
     uint32_t colors_used{ 0 };               // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
     uint32_t colors_important{ 0 };          // No. of colors used for displaying the bitmap. If 0 all colors are required
 };

struct BMPColorHeader {
uint32_t red_mask{ 0x00ff0000 };         // Bit mask for the red channel
uint32_t green_mask{ 0x0000ff00 };       // Bit mask for the green channel
uint32_t blue_mask{ 0x000000ff };        // Bit mask for the blue channel
uint32_t alpha_mask{ 0xff000000 };       // Bit mask for the alpha channel
uint32_t color_space_type{ 0x73524742 }; // Default "sRGB" (0x73524742)
uint32_t unused[16]{ 0 };                // Unused data for sRGB color space

};

void write_bmp(std::string file_name, unsigned int*p_buffer, int width, int height, int bpp)
{
	CImage myimage;

	myimage.Create(width, height, 32);
	

	for (int h = 0; h < (height); h++)
		for (int w = 0; w < (width); w++)
		{
			BYTE b = (BYTE)(p_buffer[(h*width) + w] / 10);
			myimage.SetPixelRGB(w, height - h - 1, b, b, b);
		}

	LPCTSTR str = L"test3.bmp";
	
	myimage.Save(str);

}

void write_bmp_usable(sar_field_big*p_buffer, int width, int height)
{
	CImage myimage;

	myimage.Create(width, height, 32);


	for (int h = 0; h < (height); h++)
		for (int w = 0; w < (width); w++)
		{
			BYTE b = 0;
			if (p_buffer[(h*width) + w].is_usable == true)
				b = min(p_buffer[(h*width) + w].elevation / 10,255);
				//(BYTE)(p_buffer[(h*elev_width) + w] / 10);
			myimage.SetPixelRGB(w, height - h - 1, b, b, b);
		}

	LPCTSTR str = L"test3.bmp";

	myimage.Save(str);

}

void append_text(std::string file_in, std::string file_out)
{
	std::ifstream in_file(file_in);
	std::ofstream of_file;

	of_file.open(file_out, std::ios::app);
	std::string line_string;


	while (std::getline(in_file, line_string))
	{
		of_file << line_string << std::endl;
	}
	in_file.close();
	of_file.close();
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

	/*if (atools::fs::common::GlobeReader::isDirValid("G:\\X-Plane 11\\LittleNavMapData\\all10") == false)
	{
		std::cout << "Could not open GLOBE DATA";
		return 1;
	}


	globeReader = new atools::fs::common::GlobeReader("G:\\X-Plane 11\\LittleNavMapData\\all10");

	globeReader->openFiles();



	std::vector<forest_polygon> forest_vector;

	double lat_out = 0.0;
	double long_out = 0.0;

	bool found = getFlatPoint(forest_vector, 46.401882, 13.446077, 0.1, 0.3, lat_out, long_out);

	std::cout << lat_out << " " << long_out << found << std::endl;
	*/

	//unsigned int *p_buffer = ReadElevation("+46+013.txt.elevation.raw", 1201, 1201, 2, 1, 0);
	//write_bmp("test.bmp",p_buffer,1201,1201,2);

	/*std::vector<std::string> scenery_directories;


	LPCWSTR  directory_path = L"..\\scenery\\*";


	WIN32_FIND_DATA data;
	HANDLE hFind;
	if ((hFind = FindFirstFile(directory_path, &data)) != INVALID_HANDLE_VALUE) {
		do {
			std::wstring wstr(data.cFileName);
			std::string filename(wstr.begin(), wstr.end());

			scenery_directories.push_back(filename);
		} while (FindNextFile(hFind, &data) != 0);
		FindClose(hFind);
	}*/



	
	
	for (int index_lat = -90; index_lat <= 90; index_lat++)
	{
		for (int index_long = -180; index_long <= 180; index_long++)
		{
			std::string dsf_filename = "";
			int short_lat = (index_lat / 10) * 10;
			int short_long = (index_long / 10) * 10;

			if ((index_lat < 0) && ((index_lat % 10) != 0)) short_lat-=10;
			if ((index_long < 0) && ((index_long % 10) != 0)) short_long-=10;

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


			//dsf_filename = "..\\scenery\\" + dsf_filename + "\\" + waypoint_filename;
			dsf_filename = "..\\scenery\\" + waypoint_filename;

			std::string overlay_filename = "..\\overlay\\" + waypoint_filename;

			//waypoint_filename += ".fms";

			if (exists_test(dsf_filename + ".dsf") == true)
			{

				std::string command = "move " + dsf_filename + ".dsf " + dsf_filename + ".zip";
				system(command.c_str());

				command = "7z e " + dsf_filename + ".zip";
				system(command.c_str());

				// if indeed was a zip file
				if (exists_test(waypoint_filename + ".dsf") == true)
				{

					command = "move " + dsf_filename + ".zip " + dsf_filename + ".dsf";
					system(command.c_str());

					command = "DSFTool --dsf2text " + waypoint_filename + ".dsf " + waypoint_filename + ".txt";
					system(command.c_str());

					CreateStreetWaypointFile(waypoint_filename + ".txt", "street_" + waypoint_filename + ".fms");
					CreateUrbanWaypointFile(waypoint_filename + ".txt", "urban_" + waypoint_filename + ".fms");
					CreateSARWaypointFile(waypoint_filename + ".txt", "sar_" + waypoint_filename + ".fms", index_lat, index_long);
				}
				else
				{
					command = "move " + dsf_filename + ".zip " + dsf_filename + ".dsf";
					system(command.c_str());

					command = "DSFTool --dsf2text " + dsf_filename + ".dsf " + waypoint_filename + ".txt";
					system(command.c_str());

					// Remove for Zonephoto
					command = "DSFTool --dsf2text " + overlay_filename + ".dsf " + waypoint_filename + "ov.txt";
					system(command.c_str());

					//CreateStreetWaypointFile(waypoint_filename + ".txt", "street_" + waypoint_filename + ".fms");
					//CreateUrbanWaypointFile(waypoint_filename + ".txt", "urban_" + waypoint_filename + ".fms");

					// Remove both for Zonephoto
					append_text(waypoint_filename + "ov.txt", waypoint_filename + ".txt");
					CreateSARWaypointFile(waypoint_filename + ".txt", "sar_" + waypoint_filename + ".fms", index_lat, index_long);

					// Version for ZonePhoto
					CreateSARWaypointFile(waypoint_filename + ".txt", "sar_" + waypoint_filename + ".fms", index_lat, index_long);
				}
				//std::cout << "Evaluating: " << dsf_filename << std::endl;

				//command = "DSFTool --dsf2text " + waypoint_filename + ".dsf " + waypoint_filename + ".txt";
				//system(command.c_str());

				

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

	//CreateUrbanWaypointFile("+46+013.txt", "urban_+46+013.fms");

	
	
	//globeReader->closeFiles();

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

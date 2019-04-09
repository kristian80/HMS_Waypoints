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
#include <math.h>

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

#define HRM_FLAT_SURFACE_DIST 200.0
#define HRM_FLAT_SURFACE_ANGLE 5.0

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

	int index_lat = (int) (std::min(delta_lat / (1.0 / ((double)HRM_U_SECTOR)), ((double) HRM_U_SECTOR) - 1.0));
	int index_long = (int) (std::min(delta_long / (1.0 / ((double)HRM_U_SECTOR)), ((double)HRM_U_SECTOR) - 1.0));

	return (index_lat * HRM_U_SECTOR) + index_long;
}

void CreateUrbanWaypointFile(std::string dsf_filename, std::string waypoint_filename)
{
	std::ifstream dsf_file(dsf_filename);
	std::string line_string;

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
	bool has_urban = false;

	double latitude = HRM_INV;
	double logitude = HRM_INV;

	double steepness = 10;
};

struct forest_polygon
{
	std::vector<point> forest_polygon_points;
};

inline int GetSARSegment(double latitude, double longitude)
{
	double delta_lat = latitude - ((int)latitude);
	double delta_long = longitude - ((int)longitude);

	int index_lat = (int)(std::min(delta_lat / (1.0 / ((double)HRM_SAR_SECTOR_BIG)), ((double)HRM_SAR_SECTOR_BIG) - 1.0));
	int index_long = (int)(std::min(delta_long / (1.0 / ((double)HRM_SAR_SECTOR_BIG)), ((double)HRM_SAR_SECTOR_BIG) - 1.0));

	return (index_lat * HRM_SAR_SECTOR_BIG) + index_long;
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

void CreateSARWaypointFile(std::string dsf_filename, std::string waypoint_filename, double tile_lat, double tile_long)
{
	std::ifstream dsf_file(dsf_filename);
	std::string line_string;

	waypoint_vector.clear();
	m_per_lat = HRM_INV;
	m_per_long = HRM_INV;
	min_dist = 100;
	max_waypoints = HRM_SAR_WAYPOINTS;
	w_index = 1;

	int poly_count = 0;
	int forest_count = 0;

	double long_1 = HRM_INV;
	double lat_1 = HRM_INV;
	double long_2 = HRM_INV;
	double lat_2 = HRM_INV;

	sar_field_big *p_sar;

	p_sar = new sar_field_big[HRM_SAR_SECTOR_BIG * HRM_SAR_SECTOR_BIG];

	std::vector<forest_polygon> forest_vector;
	std::vector<point> considered_points;


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

			//if (poly_type <= forest_count)
			{
				forest_polygon current_forest;

				while ((std::getline(dsf_file, line_string)) && (end_segment == false))
				{
					std::stringstream line_stream_2(line_string);

					std::string item_1 = "";
					line_stream_2 >> item_1;

					if (item_1.compare("BEGIN_WINDING") == 0)
					{
						// Do Nothing
					}

					else if (item_1.compare("END_WINDING") == 0)
					{
						// Do Nothing
					}
					else if (item_1.compare("POLYGON_POINT") == 0)
					{
						long_1 = HRM_INV;
						lat_1 = HRM_INV;

						line_stream_2 >> long_1;
						line_stream_2 >> lat_1;

						if (poly_type <= forest_count)
						{

							if (current_forest.forest_polygon_points.size() == 0)
							{
								point current_point;
								current_point.latitutde = lat_1;
								current_point.longitude = long_1;
								current_forest.forest_polygon_points.push_back(current_point);
							}
							else
							{
								if ((current_forest.forest_polygon_points[current_forest.forest_polygon_points.size() - 1].latitutde != lat_1) &&
									(current_forest.forest_polygon_points[current_forest.forest_polygon_points.size() - 1].longitude != long_1))
								{
									point current_point;
									current_point.latitutde = lat_1;
									current_point.longitude = long_1;
									current_forest.forest_polygon_points.push_back(current_point);
								}
							}
						}
						// For Urban Polygon Points (far smaller than forests)
						else
						{
							int segment = GetSARSegment(lat_1, long_1);
							p_sar[segment].has_urban = true;
						}
					}
					else if(item_1.compare("END_POLYGON") == 0)
					{
						if ((poly_type <= forest_count) && (current_forest.forest_polygon_points.size() > 0))
						{
							forest_vector.push_back(current_forest);
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

		else if (item_1.compare("PATCH_VERTEX") == 0)
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


		}


	}

	dsf_file.close();

	for (int offset_lat = 0; offset_lat < HRM_SAR_SECTOR_BIG; offset_lat++)
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

	}




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
				point &p = considered_points[index];
				fms_file << "28 " << "J" << index << " DEP 25000.000000 " << p.latitutde << " " << p.longitude << std::endl;
			}

			fms_file.close();
		}
	}


}

bool CheckForestFree(std::vector<forest_polygon> &forest_vector, double latitude, double longitude)
{
	int result = 0;
	for (auto forest : forest_vector)
	{
		result += pnpoly_cpp(forest.forest_polygon_points, latitude, longitude);
	}

	if (result == 0) return true;
	return false;
}
/*
bool getFlatPoint(std::vector<forest_polygon> &forest_vector, double lat_1, double long_1, double delta_lat_2, double delta_long_2, double &lat_out, double &long_out, int index = 0)
{
	bool found = true;

	static float m_lat = HRM_INV;
	static float m_long = HRM_INV;
	static double m_dev = HRM_INV;

	if ((lat_per_m == HRM_INV) || (long_per_m == HRM_INV))
	{
		m_per_lat = abs(calc_distance_m(lat_1, long_1, lat_1 + 1.0, long_1));
		m_per_long = abs(calc_distance_m(lat_1, long_1, lat_1, long_1 + 1.0));

		lat_per_m = 1.0 / m_per_lat;
		long_per_m = 1.0 / m_per_long;

		m_lat = HRM_FLAT_SURFACE_DIST / m_per_lat;
		m_long = HRM_FLAT_SURFACE_DIST / m_per_long;
		m_dev = sin((HRM_FLAT_SURFACE_ANGLE * M_PI) / 180.0) * HRM_FLAT_SURFACE_DIST;// * 3.28084; // Maximum surface deviation for given landing patch
	}

	float elevation[5] = { 0,0,0,0,0 };
	
	double latitude = lat_1 + (delta_lat_2 / 2.0);
	double longitude = long_1 + (delta_long_2 / 2.0);

	//latitude = 46.511795;
	//longitude = 13.497822;

	//get elevations
	atools::geo::Pos pos1(longitude, latitude);
	elevation[0] = globeReader->getElevation(pos1);

	atools::geo::Pos pos2(longitude, latitude + m_lat);
	elevation[1] = globeReader->getElevation(pos2);

	atools::geo::Pos pos3(longitude, latitude - m_lat);
	elevation[2] = globeReader->getElevation(pos3);

	atools::geo::Pos pos4(longitude + m_long, latitude);
	elevation[3] = globeReader->getElevation(pos4);

	atools::geo::Pos pos5(longitude - m_long, latitude);
	elevation[4] = globeReader->getElevation(pos5);

	
	if (!(elevation[0] > atools::fs::common::OCEAN && elevation[0] < atools::fs::common::INVALID)) found = false;
	if (!(elevation[1] > atools::fs::common::OCEAN && elevation[0] < atools::fs::common::INVALID)) found = false;
	if (!(elevation[2] > atools::fs::common::OCEAN && elevation[0] < atools::fs::common::INVALID)) found = false;
	if (!(elevation[3] > atools::fs::common::OCEAN && elevation[0] < atools::fs::common::INVALID)) found = false;
	if (!(elevation[4] > atools::fs::common::OCEAN && elevation[0] < atools::fs::common::INVALID)) found = false;



	if (abs(elevation[0] - elevation[1]) > m_dev) found = false;
	if (abs(elevation[0] - elevation[2]) > m_dev) found = false;
	if (abs(elevation[0] - elevation[3]) > m_dev) found = false;
	if (abs(elevation[0] - elevation[4]) > m_dev) found = false;

	if (	(elevation[0] == elevation[1]) &&
			(elevation[0] == elevation[2]) &&
			(elevation[0] == elevation[3]) &&
			(elevation[0] == elevation[4])) found = false;

	if (found == true) found = CheckForestFree(forest_vector, longitude, latitude);

	index++;

	if (found == true)
	{
		lat_out = latitude;
		long_out = longitude;

		std::ofstream fms_file;
		fms_file.open("test.fms");
		fms_file.precision(9);

		fms_file << "I" << std::endl;
		fms_file << "1100 Version" << std::endl;
		fms_file << "CYCLE " << 1809 << std::endl;
		fms_file << "DEP " << std::endl;
		fms_file << "DES " << std::endl;
		fms_file << "NUMENR 3" << std::endl;

		fms_file << "28 " << "J" << index << " DEP 25000.000000 " << latitude << " " << longitude << std::endl;
		fms_file << "28 " << "J" << index << " DEP 25000.000000 " << latitude + m_lat << " " << longitude << std::endl;
		fms_file << "28 " << "J" << index << " DEP 25000.000000 " << latitude - m_lat << " " << longitude << std::endl;
		fms_file << "28 " << "J" << index << " DEP 25000.000000 " << latitude << " " << longitude + m_long << std::endl;
		fms_file << "28 " << "J" << index << " DEP 25000.000000 " << latitude << " " << longitude - m_long << std::endl;
	
		fms_file.close();
		

	}
	else if ((index < 5) && (found == false))
	{
		if (found == false) found = getFlatPoint(forest_vector, lat_1, long_1, delta_lat_2 / 2.0, delta_long_2 / 2.0, lat_out, long_out, index);
		if (found == false) found = getFlatPoint(forest_vector, lat_1 + (delta_lat_2 / 2.0), long_1, delta_lat_2 / 2.0, delta_long_2 / 2.0, lat_out, long_out, index);
		if (found == false) found = getFlatPoint(forest_vector, lat_1, long_1 + (delta_long_2 / 2.0), delta_lat_2 / 2.0, delta_long_2 / 2.0, lat_out, long_out, index);
		if (found == false) found = getFlatPoint(forest_vector, lat_1 + (delta_lat_2 / 2.0), long_1 + (delta_long_2 / 2.0), delta_lat_2 / 2.0, delta_long_2 / 2.0, lat_out, long_out, index);
	}

	return found;

}*/




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

				CreateStreetWaypointFile(waypoint_filename + ".txt", "street_" + waypoint_filename + ".fms");
				CreateUrbanWaypointFile(waypoint_filename + ".txt", "urban_" + waypoint_filename + ".fms");

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

	//CreateSARWaypointFile("+46+013.txt", "sar_+46+013.fms", 46, 13);
	
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

/**
 * File:   PedDistributor.cpp
 *
 * Created on 12. October 2010, 10:52
 *
 * @section LICENSE
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 *
 *
 *
 */

#include "PedDistributor.h"
#include "../general/xmlParser.h"

/************************************************************
 StartDistributionRoom
 ************************************************************/
StartDistributionRoom::StartDistributionRoom() {
	pRoomCaption = "no caption";
	pN = -1;
}

StartDistributionRoom::StartDistributionRoom(const StartDistributionRoom& orig) {
	pRoomCaption = orig.GetRoomCaption();
	pN = orig.GetAnz();
}

StartDistributionRoom::~StartDistributionRoom() {
}

// Getter-Funktione

string StartDistributionRoom::GetRoomCaption() const {
	return pRoomCaption;
}

int StartDistributionRoom::GetAnz() const {
	return pN;
}


// Setter-Funktionen

void StartDistributionRoom::SetRoomCaption(string caption) {
	pRoomCaption = caption;
}

void StartDistributionRoom::SetAnz(int N) {
	pN = N;
}


/************************************************************
 StartDistributionSubRoom
 ************************************************************/
StartDistributionSubroom::StartDistributionSubroom() : StartDistributionRoom() {
	pSubroomID = -1;
}

StartDistributionSubroom::StartDistributionSubroom(const StartDistributionSubroom& orig) : StartDistributionRoom(orig) {
	pSubroomID = orig.GetSubroomID();
}

StartDistributionSubroom::~StartDistributionSubroom() {
}

int StartDistributionSubroom::GetSubroomID() const {
	return pSubroomID;
}

// Setter-Funktionen

void StartDistributionSubroom::SetSubroomID(int i) {
	pSubroomID = i;
}


/************************************************************
 PedDistributor
 ************************************************************/

PedDistributor::PedDistributor() {
	pv0 = new Equal(1.24, 0.26);
	pBmax = new Equal(0.25, 0.001);
	pBmin = new Equal(0.2, 0.001);
	pAtau = new Equal(0.53, 0.001);
	pAmin = new Equal(0.18, 0.001);
	pTau = new Equal(0.5, 0.001);
}

PedDistributor::PedDistributor(double v0mu, double v0sigma, double BmaxMu, double BmaxSigma,
		double BminMu, double BminSigma, double AtauMu, double AtauSigma, double AminMu,
		double AminSigma, double tauMu, double tauSigma) {
	pv0 = new Equal(v0mu, v0sigma);
	pBmax = new Equal(BmaxMu, BmaxSigma);
	pBmin = new Equal(BminMu, BminSigma);
	pAtau = new Equal(AtauMu, AtauSigma);
	pAmin = new Equal(AminMu, AminSigma);
	pTau = new Equal(tauMu, tauSigma);
	start_dis = vector<StartDistributionRoom > ();
	start_dis_sub = vector<StartDistributionSubroom> ();
}

PedDistributor::PedDistributor(const PedDistributor& orig) {
	pv0 = orig.GetV0();
	pBmax = orig.GetBmax();
	pBmin = orig.GetBmin();
	pAtau = orig.GetAtau();
	pAmin = orig.GetAmin();
	pTau = orig.GetTau();
}

PedDistributor::~PedDistributor() {
	delete pv0;
	delete pBmax;
	delete pBmin;
	delete pAtau;
	delete pAmin;
	delete pTau;
}

// Getter-Funktionen

Distribution* PedDistributor::GetV0() const {
	return pv0;
}

Distribution* PedDistributor::GetBmax() const {
	return pBmax;
}

Distribution* PedDistributor::GetBmin() const {
	return pBmin;
}

Distribution* PedDistributor::GetAtau() const {
	return pAtau;
}

Distribution* PedDistributor::GetAmin() const {
	return pAmin;
}

Distribution* PedDistributor::GetTau() const {
	return pTau;
}

void PedDistributor::InitDistributor(string filename){

	pInitialisationFile=filename;

	XMLNode xMainNode=XMLNode::openFileHelper(filename.c_str(),"persons");
	Log->write("INFO: \tLoading and parsing the persons file");

	//get the distribution node
	XMLNode xDist=xMainNode.getChildNode("distribution");
	int nDist=xDist.nChildNode("dist");
	for(int i=0;i<nDist;i++){
		XMLNode path=xDist.getChildNode("dist",i);

		//FIXME: id oder caption
		//int room_id=atoi(path.getAttribute("room_id"));
		string room_caption=path.getAttribute("room_caption");
		int number=atoi(path.getAttribute("number"));

		if(path.getAttribute("subroom_id")){
			int subroom_id=atoi(path.getAttribute("subroom_id"));
			StartDistributionSubroom dis = StartDistributionSubroom();
			dis.SetRoomCaption(room_caption);
			dis.SetSubroomID(subroom_id);
			dis.SetAnz(number);
			start_dis_sub.push_back(dis);
		}else{
			StartDistributionRoom dis = StartDistributionRoom();
			dis.SetRoomCaption(room_caption);
			dis.SetAnz(number);
			start_dis.push_back(dis);
		}
	}
	Log->write("INFO: \t done with loading and parsing the persons file");
}

int PedDistributor::Distribute(Building* building) const {

	Log->write("INFO: \tInit Distribute");

	int nPeds = 0;
	char tmp[CLENGTH];

	//first compute all possible positions in the geometry
	vector<vector< vector<Point > > > allFreePos = vector<vector< vector<Point > > >();
	for (int r = 0; r < building->GetAnzRooms(); r++) {
		vector< vector<Point > >  allFreePosRoom = vector< vector<Point > > ();
		Room* room = building->GetRoom(r);
		for (int s = 0; s < room->GetAnzSubRooms(); s++) {
			SubRoom* subr = room->GetSubRoom(s);
			allFreePosRoom.push_back(PossiblePositions(subr));
		}
		allFreePos.push_back(allFreePosRoom);
	}

	// first do the distribution in the subrooms (if any)
	int pid = 1; // the pedID is being increased throughout...
	for (int i = 0; i < (int) start_dis_sub.size(); i++) {

		string room_caption = start_dis_sub[i].GetRoomCaption();
		Room* r = building->GetRoom(room_caption);
		if(!r) continue;

		int roomID = r->GetRoomID();

		int subroomID = start_dis_sub[i].GetSubroomID();
		int N = start_dis_sub[i].GetAnz();
		if (N <= 0) {
			Log->write("ERROR: \t negative  (or null ) number of pedestrians!");
			exit(0);
		}

		vector<Point> &allpos = allFreePos[roomID][subroomID];
		int max_pos = allpos.size();
		if (max_pos < N) {
			sprintf(tmp, "ERROR: \tVerteilung von %d Fußgängern in Room %d nicht möglich! Maximale Anzahl: %d\n",
					N, roomID, allpos.size());
			Log->write(tmp);
			exit(0);
		} else {
			sprintf(tmp, "INFO: \tVerteilung von %d Fußgängern in [%d/%d]! Maximale Anzahl: %d", N, roomID, subroomID, max_pos);
			Log->write(tmp);
		}
		// Befüllen
		SubRoom* sr = building->GetRoom(roomID)->GetSubRoom(subroomID);
		DistributeInSubRoom(sr, N, allpos, roomID, &pid, building->GetRouting());
		nPeds += N;
	}

	// then do the distribution in the room

	for (int i = 0; i < (int) start_dis.size(); i++) {
		string room_caption = start_dis[i].GetRoomCaption();
		Room* r = building->GetRoom(room_caption);
		if(!r) continue;
		int N = start_dis[i].GetAnz();
		if (N <= 0) {
			Log->write("ERROR: \t negative number of pedestrians! Ignoring");
			continue;
		}

		int roomID=r->GetRoomID();
		double sum_area = 0;
		int max_pos = 0;
		double ppm; // pedestrians per square meter
		int ges_anz = 0;
		vector<int> max_anz = vector<int>();
		vector<int> akt_anz = vector<int>();

		vector< vector<Point > >&  allFreePosInRoom=allFreePos[roomID];
		for (int i = 0; i < r->GetAnzSubRooms(); i++) {
			SubRoom* sr = r->GetSubRoom(i);
			double area = sr->GetArea();
			sum_area += area;
			int anz = allFreePosInRoom[i].size();
			max_anz.push_back(anz);
			max_pos += anz;
		}
		if (max_pos < N) {
			sprintf(tmp, "ERROR: \t Distribution of %d pedestrians in Room %d not possible! Maximale number: %d\n",
					N, r->GetRoomID(), max_pos);
			Log->write(tmp);
			exit(0);
		}
		ppm = N / sum_area;
		// Anzahl der Personen pro SubRoom bestimmen
		for (int i = 0; i < r->GetAnzSubRooms(); i++) {
			SubRoom* sr = r->GetSubRoom(i);
			int anz = sr->GetArea() * ppm + 0.5; // wird absichtlich gerundet
			while (anz > max_anz[i]) {
				anz--;
			}
			akt_anz.push_back(anz);
			ges_anz += anz;
		}
		// Falls N noch nicht ganz erreicht, von vorne jeweils eine Person dazu
		int j = 0;
		while (ges_anz < N) {
			if (akt_anz[j] < max_anz[j]) {
				akt_anz[j] = akt_anz[j] + 1;
				ges_anz++;
			}
			j = (j + 1) % max_anz.size();
		}
		j = 0;
		while (ges_anz > N) {
			if (akt_anz[j] > 0) {
				akt_anz[j] = akt_anz[j] - 1;
				ges_anz--;
			}
			j = (j + 1) % max_anz.size();
		}
		// distributing
		for (unsigned int i = 0; i < akt_anz.size(); i++) {
			SubRoom* sr = r->GetSubRoom(i);
			if (akt_anz[i] > 0)
				DistributeInSubRoom(sr, akt_anz[i], allFreePosInRoom[i], r->GetRoomID(), &pid, building->GetRouting());
		}
		nPeds += N;
	}


	// now assign individual attributes

	XMLNode xMainNode=XMLNode::openFileHelper(pInitialisationFile.c_str(),"persons");
	Log->write("INFO: \tLoading and parsing the persons file");

	//get the distribution node
	int nPersons=xMainNode.nChildNode("person");
	for(int i=0;i<nPersons;i++){
		XMLNode xPerson=xMainNode.getChildNode("person",i);
		int id=xmltoi(xPerson.getAttribute("id"),-1);

		if(id==-1){
			Log->write("ERROR:\tin the person attribute file. The id is mandatory ! skipping the entry");
			continue;
		}
		//look for that pedestrian.
		Pedestrian* ped=NULL;
		for (int i = 0; i < building->GetAnzRooms(); i++) {
			Room* room = building->GetRoom(i);
			for (int j = 0; j < room->GetAnzSubRooms(); j++) {
				SubRoom* sub = room->GetSubRoom(j);
				for (int k = 0; k < sub->GetAnzPedestrians(); k++) {
					Pedestrian* p=sub->GetPedestrian(k);
					if(p->GetPedIndex()==id){
						ped=p;
						goto END;
					}
				}
			}
		}
		END:
		if(!ped){
			sprintf(tmp, "WARNING: \t Ped [%d] does not not exit yet. I am creating a new one",id);
			Log->write(tmp);
			ped=new Pedestrian();
			ped->SetPedIndex(id);

			// a und b setzen muss vor v0 gesetzt werden, da sonst v0 mit Null überschrieben wird
			Ellipse E = Ellipse();
			double atau = GetAtau()->GetRand();
			double amin = GetAmin()->GetRand();
			E.SetAv(atau);
			E.SetAmin(amin);
			double bmax = GetBmax()->GetRand();
			double bmin = GetBmin()->GetRand();
			E.SetBmax(bmax);
			E.SetBmin(bmin);
			ped->SetEllipse(E);
			// tau
			double tau = GetTau()->GetRand();
			ped->SetTau(tau);
			// V0
			double v0 = GetV0()->GetRand();
			ped->SetV0Norm(v0);
			nPeds++;
		}

		double height=xmltof(xPerson.getAttribute("height"),-1);
		if( height!=-1){
			ped->SetHeight(height);
		}

		double age=xmltof(xPerson.getAttribute("age"),-1);
		if( age!=-1){
			ped->SetAge(age);
		}

		string gender=xmltoa(xPerson.getAttribute("gender"),"-1");
		if( gender!="-1"){
			ped->SetGender(gender);
		}

		double goal_id=xmltof(xPerson.getAttribute("goal"),-1);
		if( goal_id!=-1){
			if((ped->GetFinalDestination()!=FINAL_DEST_OUT ) &&
					(ped->GetFinalDestination()!=goal_id)){
				sprintf(tmp, "ERROR: \tconflicting final destination for Ped [%d]", ped->GetPedIndex());
				Log->write(tmp);
				sprintf(tmp, "ERROR: \talready assigned to destination [%d]", ped->GetFinalDestination());
				Log->write(tmp);
				exit(EXIT_FAILURE);
			}
			ped->SetFinalDestination(goal_id);
			building->GetRouting()->AddFinalDestinationID(goal_id);
		}

		double wishVelo=xmltof(xPerson.getAttribute("wishVelo"),-1);
		if( wishVelo!=-1){
			ped->SetV0Norm(wishVelo);
		}

		double startX=xmltof(xPerson.getAttribute("startX"),-1);
		double startY=xmltof(xPerson.getAttribute("startY"),-1);

		if(startX!=-1 && startY!=-1){
			ped->SetPos(Point(startX,startY));
			//in that case the room should be automatically adjusted
			for (int i = 0; i < building->GetAnzRooms(); i++) {
				Room* room = building->GetRoom(i);
				for (int j = 0; j < room->GetAnzSubRooms(); j++) {
					SubRoom* sub = room->GetSubRoom(j);
					if(sub->IsInSubRoom(Point(startX,startY))){
						//if a room was already assigned
						if(ped->GetRoomID()!=-1){
							if(FindPedAndDeleteFromRoom(building,ped)){
								sprintf(tmp, "WARNING: \t Ped [%d] does not not exist yet , will be created and moved to the corresponding room.", id);
								Log->write(tmp);
							}
						}
						ped->SetRoomID(room->GetRoomID(),room->GetCaption());
						ped->SetSubRoomID(sub->GetSubRoomID());
						sub->AddPedestrian(ped);
					}
				}
			}
		}
	}

	int nPedsExpected= xmltoi(xMainNode.getChildNode("header").getChildNode("number").getText(),-1);

	//now parse the different groups
	XMLNode xGroups=xMainNode.getChildNode("groups");
	int nGroup=xGroups.nChildNode("group");
	for(int i=0;i<nGroup;i++){
		XMLNode group=xGroups.getChildNode("group",i);
		int group_id=xmltoi(group.getAttribute("id"),-1);
		int trip_id=xmltoi(group.getChildNode("trip").getText(),-1);
		int goal_id=xmltoi(group.getChildNode("goal").getText(),-1);

		if((goal_id !=-1) && (trip_id!=-1)){
			sprintf(tmp, "ERROR: \ttrip and goal cannot be set for the same group [%d] !",group_id);
			Log->write(tmp);
			sprintf(tmp, "ERROR: \tas they might conflict!");
			Log->write(tmp);
			exit(EXIT_FAILURE);
		}

		//get the members
		string members=group.getChildNode("members").getText();

		char* str = (char*) members.c_str();
		char *p = strtok(str, ",");
		while (p) {
			int ped_id=xmltoi(p);
			Pedestrian* ped=building->GetPedestrian(ped_id);
			if(ped){
				ped->SetGroup(group_id);
				if(trip_id!=-1){
					ped->SetTrip(building->GetRouting()->GetTrip(trip_id));
					sprintf(tmp, "ERROR: \tTrip is actually not supported for pedestrian [%d]. Please use <goal><goal/> instead", ped->GetPedIndex());
					Log->write(tmp);
					exit(EXIT_FAILURE);
				}
				if(goal_id!=-1){
					if((ped->GetFinalDestination()!=FINAL_DEST_OUT ) &&
							(ped->GetFinalDestination()!=goal_id)){
						sprintf(tmp, "ERROR: \tconflicting final destinations for Ped [%d]", ped->GetPedIndex());
						Log->write(tmp);
						sprintf(tmp, "ERROR: \talready assigned to a destination with ID [%d]", ped->GetFinalDestination());
						Log->write(tmp);
						exit(EXIT_FAILURE);
					}

					ped->SetFinalDestination(goal_id);
					building->GetRouting()->AddFinalDestinationID(goal_id);
				}
			}else{
				sprintf(tmp, "ERROR:\tID [%d] out of range. The largest allowed ID based on the current distribution is [%d]",ped_id,building->GetAnzPedestrians());
				Log->write(tmp);
				sprintf(tmp, "ERROR:\tVerify that you have distributed all [%d] pedestrians",nPedsExpected);
				Log->write(tmp);
				exit(EXIT_FAILURE);
			}
			p = strtok(NULL, ",");
		}
	}


	//now do the last check if all pedestrians were distributed:

	if(nPedsExpected!=nPeds){
		sprintf(tmp, "ERROR:\tThe number of distributed pedestrians [%d] does not match \n"
				"        \tthe total number of specified pedestrians in the header [%d]!",nPeds,nPedsExpected);
		Log->write(tmp);
		exit(EXIT_FAILURE);
	}

	return nPeds;
}

bool PedDistributor::FindPedAndDeleteFromRoom(Building* building,Pedestrian*ped) const {

	for (int i = 0; i < building->GetAnzRooms(); i++) {
		Room* room = building->GetRoom(i);
		for (int j = 0; j < room->GetAnzSubRooms(); j++) {
			SubRoom* sub = room->GetSubRoom(j);
			for (int k = 0; k < sub->GetAnzPedestrians(); k++) {
				Pedestrian* p=sub->GetPedestrian(k);
				if(p->GetPedIndex()==ped->GetPedIndex()){
					sub->DeletePedestrian(k);
					return true;
				}
			}
		}
	}
	return false;
}


vector<Point> PedDistributor::PositionsOnFixX(double min_x, double max_x, double min_y, double max_y,
		SubRoom* r, double bufx, double bufy, double dy) const {
	vector<Point> positions;
	double x = (max_x + min_x)*0.5;
	double y = min_y;

	while (y < max_y) {
		Point pos = Point(x, y);
		// Abstand zu allen Wänden prüfen
		int k;
		for (k = 0; k < r->GetAnzWalls(); k++) {
			Wall w = r->GetWall(k);
			if (w.DistTo(pos) < max(bufx, bufy) || !r->IsInSubRoom(pos)) {
				break; // Punkt ist zu nah an einer Wand oder nicht im Raum => ungültig
			}
		}
		if (k == r->GetAnzWalls()) {
			//check all transitions
			bool tooNear=false;
			for(unsigned int t=0;t<r->GetAllTransitions().size();t++){
				if(r->GetTransition(t)->DistTo(pos)<J_EPS_GOAL){
					//too close
					tooNear=true;
					break;
				}
			}
			if(tooNear==false) positions.push_back(pos);

			//reset and check all crossings
			tooNear=false;
			for(unsigned int c=0;c<r->GetAllCrossings().size();c++){
				if(r->GetCrossing(c)->DistTo(pos)<J_EPS_GOAL){
					//too close
					tooNear=true;
					break;
				}
			}
			if(tooNear==false) positions.push_back(pos);
		}
		y += dy;
	}
	return positions;

}

vector<Point>PedDistributor::PositionsOnFixY(double min_x, double max_x, double min_y, double max_y,
		SubRoom* r, double bufx, double bufy, double dx) const {
	vector<Point> positions;
	double y = (max_y + min_y)*0.5;
	double x = min_x;

	while (x < max_x) {
		Point pos = Point(x, y);
		// check distance to wall
		int k;
		for (k = 0; k < r->GetAnzWalls(); k++) {
			Wall w = r->GetWall(k);
			if (w.DistTo(pos) < max(bufx, bufy) || !r->IsInSubRoom(pos)) {
				break; // Punkt ist zu nah an einer Wand oder nicht im Raum => ungültig
			}
		}
		if (k == r->GetAnzWalls()) {
			//check all transitions
			bool tooNear=false;
			for(unsigned int t=0;t<r->GetAllTransitions().size();t++){
				if(r->GetTransition(t)->DistTo(pos)<J_EPS_GOAL){
					//too close
					tooNear=true;
					break;
				}
			}
			if(tooNear==false) positions.push_back(pos);

			//reset and check all crossings
			tooNear=false;
			for(unsigned int c=0;c<r->GetAllCrossings().size();c++){
				if(r->GetCrossing(c)->DistTo(pos)<J_EPS_GOAL){
					//too close
					tooNear=true;
					break;
				}
			}
			if(tooNear==false) positions.push_back(pos);
		}
		x += dx;
	}
	return positions;
}

vector<Point> PedDistributor::PossiblePositions(SubRoom* r) const {
	double uni = 0.7; // wenn ein Raum in x oder y -Richtung schmaler ist als 0.7 wird in der Mitte verteilt
	double bufx = 0.12;
	double bufy = 0.12;

	double dx = GetAmin()->GetMean() + bufx;
	double dy = GetBmax()->GetMean() + bufy;

	vector<double>::iterator min_x, max_x, min_y, max_y;
	vector<Point> poly = r->GetPolygon();
	vector<Point> positions;
	vector<double> xs;
	vector<double> ys;

	for (int p = 0; p < (int) poly.size(); ++p) {
		xs.push_back(poly[p].GetX());
		ys.push_back(poly[p].GetY());
	}

	min_x = min_element(xs.begin(), xs.end());
	max_x = max_element(xs.begin(), xs.end());
	min_y = min_element(ys.begin(), ys.end());
	max_y = max_element(ys.begin(), ys.end());

	if (*max_y - *min_y < uni) {
		positions = PositionsOnFixY(*min_x, *max_x, *min_y, *max_y, r, bufx, bufy, dx);
	} else if (*max_x - *min_x < uni) {
		positions = PositionsOnFixX(*min_x, *max_x, *min_y, *max_y, r, bufx, bufy, dy);
	} else {
		// create the grid
		double x = (*min_x);
		while (x < *max_x) {
			double y = (*min_y);
			while (y < *max_y) {
				y += dy;
				Point pos = Point(x, y);
				bool tooNear=false;

				// check the distance to all Wall
				for (int k = 0; k < r->GetAnzWalls(); k++) {
					const Wall& w = r->GetWall(k);
					if (w.DistTo(pos) < max(bufx, bufy) || !r->IsInSubRoom(pos)) {
						tooNear=true;
						break; // too close
					}
				}

				//check all transitions
				if(tooNear==true) continue;
				for(unsigned int t=0;t<r->GetAllTransitions().size();t++){
					if(r->GetTransition(t)->DistTo(pos)<max(bufx, bufy)){
						//too close
						tooNear=true;
						break;
					}
				}

				//  and check all crossings
				if(tooNear==true) continue;
				for(unsigned int c=0;c<r->GetAllCrossings().size();c++){
					if(r->GetCrossing(c)->DistTo(pos)<max(bufx, bufy)){
						//too close
						tooNear=true;
						break;
					}
				}


				// and finally all opened obstacles
				if(tooNear==true) continue;

				const vector<Obstacle*>& obstacles = r->GetAllObstacles();
				for (unsigned int obs = 0; obs < obstacles.size(); ++obs) {
					Obstacle *obst =obstacles[obs];
					const vector<Wall>& walls = obst->GetAllWalls();
					for (unsigned int i = 0; i < walls.size(); i++) {
						if (walls[i].DistTo(pos) < max(bufx, bufy) || !r->IsInSubRoom(pos)) {
							tooNear=true;
							break; // too close
						}
					}

					//only continue if...
					if(tooNear==true) continue;

					if((obst->GetClosed()==1) && (obst->Contains(pos)==true)){
						tooNear=true;
						break; // too close
					}
				}


				if(tooNear==false) positions.push_back(pos);

			}
			x += dx;
		}
	}

	return positions;
}
/* Verteilt N Fußgänger in SubRoom r
 * Algorithms:
 *   - Lege Gitter von min_x bis max_x mit Abstand dx und entsprechend min_y bis max_y mit
 *     Abstand dy
 *   - Prüfe alle so enstandenen Gitterpunkte, ob ihr Abstand zu Wänden und Übergängen aus
 *     reicht
 *   - Wenn nicht verwerfe Punkt, wenn ja merke Punkt in Vector positions
 *   - Wähle zufällig einen Punkt aus dem Vektor aus Position der einzelnen Fußgänger
 * Parameter:
 *   - r:       SubRoom in den verteilt wird
 *   - N:       Anzahl der Fußgänger für SubRoom r
 *   - pid:     fortlaufender Index der Fußgänger (wird auch wieder zurückgegeben, für den
 *              nächsten Aufruf)
 *   - routing: wird benötigt um die Zielline der Fußgänger zu initialisieren
 * */
void PedDistributor::DistributeInSubRoom(SubRoom* r, int N, vector<Point>& positions, int roomID, int* pid, Routing * routing) const {
	char tmp[CLENGTH];
	int anz = positions.size();

	if (anz < N) {
		sprintf(tmp, "ERROR: \tVerteilung von %d Fußgängern in Subroom %d nicht möglich! Maximale Anzahl: %d\n",
				N, r->GetSubRoomID(), anz);
		Log->write(tmp);
		exit(0);
	} else {
		sprintf(tmp, "\t\tVerteilung von %d Fußgängern! Maximale Anzahl: %d",
				N, anz);
		Log->write(tmp);
	}
	// set the pedestrians
	for (int i = 0; i < N; ++i) {

		Pedestrian* ped = new Pedestrian();
		// PedIndex
		ped->SetPedIndex(*pid);
		// a und b setzen muss vor v0 gesetzt werden, da sonst v0 mit Null überschrieben wird
		Ellipse E = Ellipse();
		double atau = GetAtau()->GetRand();
		double amin = GetAmin()->GetRand();
		E.SetAv(atau);
		E.SetAmin(amin);
		double bmax = GetBmax()->GetRand();
		double bmin = GetBmin()->GetRand();
		E.SetBmax(bmax);
		E.SetBmin(bmin);
		ped->SetEllipse(E);
		// tau
		double tau = GetTau()->GetRand();
		ped->SetTau(tau);
		// V0
		double v0 = GetV0()->GetRand();
		ped->SetV0Norm(v0);
		// Position
		int index = rand() % positions.size();
		Point pos = positions[index];
		ped->SetPos(pos);
		positions.erase(positions.begin() + index);
		// roomID
		ped->SetRoomID(roomID,"");
		// SubRoomID
		ped->SetSubRoomID(r->GetSubRoomID());
		// setzen
		r->AddPedestrian(ped);
		(*pid)++;
	}

}


string PedDistributor::writeParameter() const {
	string s;
	char tmp[CLENGTH];

	s.append("\tParameter der Fußgänger:\n");
	sprintf(tmp, "\t\tv0 ~ N(%f, %f)\n", GetV0()->GetMean(), GetV0()->GetSigma());
	s.append(tmp);
	sprintf(tmp, "\t\tb_max ~ N(%f, %f)\n", GetBmax()->GetMean(), GetBmax()->GetSigma());
	s.append(tmp);
	sprintf(tmp, "\t\tb_min ~ N(%f, %f)\n", GetBmin()->GetMean(), GetBmin()->GetSigma());
	s.append(tmp);
	sprintf(tmp, "\t\ta_min ~ N(%f, %f)\n", GetAmin()->GetMean(), GetAmin()->GetSigma());
	s.append(tmp);
	sprintf(tmp, "\t\ta_tau ~ N(%f, %f)\n", GetAtau()->GetMean(), GetAtau()->GetSigma());
	s.append(tmp);
	sprintf(tmp, "\t\ttau ~ N(%f, %f)\n", GetTau()->GetMean(), GetTau()->GetSigma());
	s.append(tmp);

	return s;
}

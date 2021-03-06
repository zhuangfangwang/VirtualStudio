#include "simulation_window.h"

#include "scene.h"
#include "AVIGenerator.h"
#include "animation_editor_widget.h"

#include <QtGui>
#include <QtWidgets/QtWidgets>
#include <QOpenGLShaderProgram>
#include <cassert>

SimulationWindow::SimulationWindow( Scene* scene, QWindow* screen )
	: QWindow( screen ), 
	  scene_( scene )
{	
	// Tell Qt we will use OpenGL for this window
	setSurfaceType(OpenGLSurface);

	// Specify the format we wish to use
	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setMajorVersion(4);
	format.setMinorVersion(0);
	format.setSamples(4);
	format.setProfile(QSurfaceFormat::CoreProfile);
	
	setFormat(format);
	create();
 
	// Create an OpenGL context
	context_ = new QOpenGLContext;
	context_->setFormat(format);
	context_->create();
 
	// Setup our scene
	context_->makeCurrent(this);
	scene_->setContext(context_);
	initializeGL();
 
	// Make sure we tell OpenGL about new window sizes
	connect( this, SIGNAL( widthChanged( int ) ), this, SLOT( resizeGL() ) );
	connect( this, SIGNAL( heightChanged( int ) ), this, SLOT( resizeGL() ) );
}

void SimulationWindow::initializeGL()
{
	context_->makeCurrent(this);
	scene_->initialize();
}

void SimulationWindow::resizeGL()
{
	context_->makeCurrent(this);
	scene_->resize(width(), height());
	paintGL();
}


void SimulationWindow::paintGL()
{
	if (isExposed()) {
		// Make the context current
		context_->makeCurrent(this);

		// Do the rendering (to the back buffer)
		scene_->render();

		// Swap front/back buffers
		context_->swapBuffers(this);
	}
}

void SimulationWindow::mousePressEvent( QMouseEvent *event )
{
	if (event->button() == Qt::LeftButton) {
		cur_pos_ = prev_pos_ = event->pos();
	}
	
	QWindow::mousePressEvent(event);
}

void SimulationWindow::mouseMoveEvent( QMouseEvent *event )
{
	if (event->buttons() & Qt::LeftButton) {
		cur_pos_ = event->pos();
		float dx = -12.0f * float(cur_pos_.x() - prev_pos_.x()) / width();
		float dy = -12.0f * float(cur_pos_.y() - prev_pos_.y()) / height();
		
		switch (scene_->interactionMode()) {
		case Scene::SELECT: 
			break;
		case Scene::ROTATE: 
			scene_->rotate(prev_pos_, cur_pos_);
			break;
		case Scene::PAN: 
			scene_->pan(dx, dy);
			break;
		case Scene::ZOOM: 
			scene_->zoom(dy*120);
			break;
		case Scene::CLOTH_ROTATE: 
			scene_->cloth_rotate(prev_pos_, cur_pos_);
			break;
		case Scene::CLOTH_MOVE: 
			scene_->cloth_move(dx, dy);
			break;
		case Scene::CLOTH_SCALE: 
			scene_->cloth_scale(dy);
			break;
		default: 
			break;
		}
		paintGL();
		prev_pos_ = cur_pos_;
	}

	QWindow::mouseMoveEvent(event);
}

void SimulationWindow::wheelEvent( QWheelEvent *event )
{
	// wheel: zoom
	if (event->isAccepted()) {
		scene_->zoom(event->delta());
		event->accept();
		paintGL();
	}
	QWindow::wheelEvent(event);
}

void SimulationWindow::updateAnimation(int frame)
{
	// 更新Avatar动画
	scene_->updateAvatarAnimation(frame);
	// 更新Cloth动画，wunf
	if(scene_->is_replay())
		scene_->updateClothAnimation(frame);
	paintGL();
}

void SimulationWindow::restoreToBindpose()
{
	scene_->restoreToBindpose();
	paintGL();
}

void SimulationWindow::startSimulate()
{

	int total_frame = scene_->totalFrame();
	bool inited = false;

	//QString file_name = QFileDialog::getSaveFileName(NULL, tr("Save As AVI"),  ".", tr("AVI files (*.avi)"));

	//AviGen = new AVIGenerator;

	//// set 15fps
	//AviGen->SetRate(15);
	//
	//// give info about bitmap
	//AviGen->SetBitmapHeader(width(), height());		

	//// set filename, extension ".avi" is appended if necessary
	//AviGen->SetFileName(file_name.toLocal8Bit().constData());

	//// retreiving size of image
	//lpbih=AviGen->GetBitmapHeader();

	//// allocating memory
	//bmBits=new BYTE[lpbih->biSizeImage];

	//HRESULT hr=AviGen->InitEngine();
	//if (FAILED(hr))
	//{
	//	QMessageBox::critical(0, "error", "InitEngine error!");
	//}

	int factor = RemixerWidget::getSampleInterval() / RemixerWidget::getSimInterval();
	total_frame *= factor;

	QProgressDialog process(NULL);  
	process.setLabelText(tr("simulating..."));  
	process.setRange(0, total_frame + 1);  
	process.setModal(true);  
	process.setCancelButtonText(tr("cancel"));

	for(int i = 0; i <= total_frame; ++i)
	{
		process.setValue(i + 1);
		if(process.wasCanceled())  
			break;  
		if(!inited)
		{
			scene_->updateAvatarAnimationSim(0);
			scene_->initAvatarToSimulate();
			scene_->startSimulate();
			// abandonded
			//scene_->initCmFile("cmf\\temp.cmf");
			inited = true;
		}
		else
		{
			scene_->updateAvatarAnimationSim(i);
			scene_->updateAvatarToSimulate();
			scene_->simulateStep();
		}

		if(i % factor == 0)
			scene_->writeAFrame(i / factor);
		paintGL();

		//glReadBuffer(GL_BACK);
		//glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits); 
		//// send to avi engine
		//HRESULT hr=AviGen->AddFrame(bmBits);
		//if (FAILED(hr))
		//{
		//	QMessageBox::critical(0, "error", "AddFrame error!");
		//}
		//glReadBuffer(GL_FRONT);
	}
	scene_->finishedSimulate();
	/*AviGen->ReleaseEngine();
	delete[] bmBits;
	delete AviGen;*/

	// abandonded

	/*file_name = QFileDialog::getSaveFileName(NULL, tr("Save Cloth Motion"),  ".", tr("Cloth motion files (*.cmf)"));

	if (!file_name.isEmpty()) 
	{
		cloth_motion_file_ = file_name;
		std::ifstream ifs("cmf\\temp.cmf");
		assert(ifs.is_open());
		std::ofstream ofs(file_name.toStdString());
		assert(ofs.is_open());
		ofs << ifs.rdbuf();
		ofs.close();
	}*/
	QMessageBox::information(NULL, "Simulation finished", "Simulation finished.", QMessageBox::Ok);

}

void SimulationWindow::record()
{

	QString file_name = QFileDialog::getSaveFileName(NULL, tr("Save As AVI"),  ".", tr("AVI files (*.avi)"));

	AviGen = new AVIGenerator;

	// set 15fps
	AviGen->SetRate(15);
	
	// give info about bitmap
	AviGen->SetBitmapHeader(width(), height());		

	// set filename, extension ".avi" is appended if necessary
	AviGen->SetFileName(file_name.toLocal8Bit().constData());

	// retreiving size of image
	lpbih=AviGen->GetBitmapHeader();

	// allocating memory
	bmBits=new BYTE[lpbih->biSizeImage];

	HRESULT hr=AviGen->InitEngine();
	if (FAILED(hr))
	{
		QMessageBox::critical(0, "error", "InitEngine error!");
		AviGen->ReleaseEngine();
		delete[] bmBits;
		delete AviGen;
		return;
	}

	int total_frame = scene_->totalFrame();

	QProgressDialog process(NULL);  
	process.setLabelText(tr("Recording..."));  
	process.setRange(0, total_frame);  
	process.setModal(true);  
	process.setCancelButtonText(tr("cancel"));

	for(int i = 0; i < total_frame; ++i)
	{
		updateAnimation(i);
		process.setValue(i + 1);
		if(process.wasCanceled())  
			break;

		glReadBuffer(GL_BACK);
		glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits); 
		// send to avi engine
		HRESULT hr=AviGen->AddFrame(bmBits);
		if (FAILED(hr))
		{
			QMessageBox::critical(0, "error", "AddFrame error!");
			AviGen->ReleaseEngine();
			delete[] bmBits;
			delete AviGen;
			return;
		}
		glReadBuffer(GL_FRONT);
	}
	
	AviGen->ReleaseEngine();
	delete[] bmBits;
	delete AviGen;
	QMessageBox::information(NULL, "Record finished", "Record finished.", QMessageBox::Ok);
}
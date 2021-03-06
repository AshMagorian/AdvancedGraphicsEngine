#include <VanillaEngine/VanillaEngine.h>

void ShadowManager::Init(std::weak_ptr<Application> _app) 
{ 
	m_application = _app; 
	m_shadowBox = std::make_shared<ShadowBox>();
	m_shadowBox->SetCamera(m_application.lock()->GetCamera());
	m_shadowBox->CalcFrustrumWidthsAndHeights();
	m_shader = std::make_shared<ShadowShader>();
	try
	{
		m_shader->Init("../src/resources/shaders/vertFragFiles/DepthShader.vert", "../src/resources/shaders/vertFragFiles/DepthShader.frag");
	}
	catch (Exception& e)
	{
		std::cout << "VanillaEngine Exception: " << e.what() << std::endl;
	}
	m_debugDepthShader = std::make_shared<ShaderProgram>("../src/resources/shaders/debugDepthShader.txt");

	m_shadowWidth = 2048; m_shadowHeight = 2048;

	m_depthMap = std::make_shared<RenderTexture>(m_shadowWidth, m_shadowHeight);
	m_depthMap->InitDepthOnly();
	m_depthMap->SetFilteringNearest();
	m_depthMap->SetWrapClampBorder(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void ShadowManager::CalcInitialLightViewMatrix()
{
	glm::vec3 lightDir = m_application.lock()->GetLightManager()->GetDirectionalLight()->direction;
	glm::vec3 lightPos = 3.0f * glm::normalize(-lightDir);
	m_lightViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void ShadowManager::RenderShadowMap()
{
	glViewport(0, 0, m_shadowWidth, m_shadowHeight);

	glBindFramebuffer(GL_FRAMEBUFFER, m_depthMap->GetFboId());

	glClear(GL_DEPTH_BUFFER_BIT);

	if (m_first)
	{
		CalcInitialLightViewMatrix();
		m_first = false;
	}

	m_shadowBox->Update(); 
	glm::vec3 lightDir = m_application.lock()->GetLightManager()->GetDirectionalLight()->direction;
	CalcOrthoProjectionMatrix(m_shadowBox->GetWidth(), m_shadowBox->GetHeight(), m_shadowBox->GetLength());
	CalcViewMatrix(lightDir, m_shadowBox->GetCenter());

	m_shader->SetUniforms(m_lightViewMatrix, m_lightProjectionMatrix);

	std::list<std::shared_ptr<Terrain>> terrainData = m_application.lock()->GetTerrainRenderer()->GetTerrain();
	m_shader->Draw(m_objectData, terrainData);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowManager::CalcViewMatrix(glm::vec3 _lightDir, glm::vec3 _center)
{
	glm::vec3 lightDir = glm::normalize(_lightDir);
	m_lightViewMatrix = glm::mat4(1.0f);
	float pitch = (float)acos(glm::length(glm::vec2(lightDir.x, lightDir.z)));
	m_lightViewMatrix = glm::rotate(m_lightViewMatrix, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
	float yaw = (float)glm::degrees((float)atan(lightDir.x / lightDir.z));
	yaw = lightDir.z > 0.0f ? yaw - 180.0f : yaw;
	m_lightViewMatrix = glm::rotate(m_lightViewMatrix, -glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
	
	//float offset = 5.0f;
	//glm::vec3 start = -_center + (lightDir * offset);
	//m_lightViewMatrix = glm::translate(m_lightViewMatrix, start);
	m_lightViewMatrix = glm::translate(m_lightViewMatrix, -_center);

	//std::cout << "X: " << _center.x << "Y: " << _center.y << "Z: " << _center.z << std::endl;
	//std::cout << "pitch: " << glm::degrees(pitch) << "    yaw: " << yaw << std::endl;
	//temp
	//glm::vec3 lightPos = 3.0f * glm::normalize(-_lightDir);
	//m_lightViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void ShadowManager::CalcOrthoProjectionMatrix(float _w, float _h, float _l)
{
	float offset = 80.0f;
	_l += offset;
	m_lightProjectionMatrix = glm::mat4(1.0f);
	m_lightProjectionMatrix[0][0] = 2.0f / (_w + 5.0f);
	m_lightProjectionMatrix[1][1] = 2.0f / (_h + 5.0f);
	m_lightProjectionMatrix[2][2] = -2.0f / _l;
	m_lightProjectionMatrix[3][3] = 1.0f;

	//std::cout << "w: " << _w << "  h: " << _h << " w: " << _l << std::endl;
	
	//temp
	//m_lightProjectionMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 15.0f);
	
}

void ShadowManager::DebugDepthTexture()
{
	glViewport(0, 0, 1024 / 3, 1024 / 3);

	//m_debugDepthShader->SetUniform("near_plane", 1.0f);
	//m_debugDepthShader->SetUniform("far_plane", 7.5f);

	std::shared_ptr<Texture> tex = std::static_pointer_cast<Texture>(m_depthMap);
	m_debugDepthShader->Draw(tex);
}

void ShadowManager::AddData(ShadowData _data)
{
	std::shared_ptr<Entity> e = _data.transform->GetEntity();
	for (std::list<ShadowData>::iterator i = m_objectData.begin(); i != m_objectData.end(); ++i)
	{
		if (i->transform->GetEntity() == e)
		{
			return;
		}
	}
	m_objectData.push_back(_data);
}

void ShadowManager::RemoveData(ShadowData _data)
{
	std::shared_ptr<Entity> e = _data.transform->GetEntity();
	for (std::list<ShadowData>::iterator i = m_objectData.begin(); i != m_objectData.end(); ++i)
	{
		if (i->transform->GetEntity() == e)
		{
			m_objectData.erase(i);
			return;
		}
	}
	std::cout << "Couldn't remove data, shadow data not found" << std::endl;
}
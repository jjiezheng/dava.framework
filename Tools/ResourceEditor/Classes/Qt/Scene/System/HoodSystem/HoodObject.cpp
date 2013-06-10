/*==================================================================================
    Copyright (c) 2008, DAVA, INC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of the DAVA, INC nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE DAVA, INC AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DAVA, INC BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/

#include "Scene/System/HoodSystem/HoodObject.h"

HoodObject::HoodObject(DAVA::float32 bs)
	: baseSize(bs)
{ }

HoodObject::~HoodObject()
{
	for (size_t i = 0; i < collObjects.size(); i++)
	{
		delete collObjects[i];
	}
}

void HoodObject::UpdatePos(const DAVA::Vector3 &pos)
{
	for (size_t i = 0; i < collObjects.size(); i++)
	{
		collObjects[i]->UpdatePos(pos);
	}
}

void HoodObject::UpdateScale(const DAVA::float32 &scale)
{
	for (size_t i = 0; i < collObjects.size(); i++)
	{
		collObjects[i]->UpdateScale(scale);
	}
}

HoodCollObject* HoodObject::CreateLine(const DAVA::Vector3 &from, const DAVA::Vector3 &to)
{
	HoodCollObject* ret = new HoodCollObject();

	DAVA::Vector3 direction = (to - from);
	DAVA::float32 length = direction.Length();
	DAVA::Vector3 axisX(1, 0, 0);
	DAVA::Vector3 rotateNormal;
	DAVA::float32 rotateAngle = 0;
	DAVA::float32 weight = 0.3f;

	// initially create object on x axis
	ret->btShape = new btCylinderShapeX(btVector3(length / 2, weight / 2, weight / 2)); 
	ret->btObject = new btCollisionObject();
	ret->btObject->setCollisionShape(ret->btShape);
	ret->baseFrom = from;
	ret->baseTo = to;
	ret->baseOffset = DAVA::Vector3(length / 2, 0, 0);
	ret->baseRotate.Identity();

	direction.Normalize();
	rotateNormal = axisX.CrossProduct(direction);

	// do we need rotation
	if(!rotateNormal.IsZero())
	{
		rotateNormal.Normalize();
		rotateAngle = acosf(axisX.DotProduct(direction));

		ret->baseRotate.CreateRotation(rotateNormal, -rotateAngle);
	}

	if(0 != rotateAngle)
	{
		btTransform trasf;
		trasf.setIdentity();
		trasf.setRotation(btQuaternion(btVector3(rotateNormal.x, rotateNormal.y, rotateNormal.z), rotateAngle));
		ret->btObject->setWorldTransform(trasf);
	}

	collObjects.push_back(ret);
	return ret;
}
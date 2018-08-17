#include <Editor/CollidersEditActions.hpp>

namespace obe::Editor
{
    void connectCollidersActions(
        Triggers::TriggerGroup* editorTriggers,
        Input::InputManager& inputManager,
        Scene::Scene& scene,
        System::Cursor& cursor,
        int& colliderPtGrabbed,
        Collision::PolygonalCollider*& selectedMasterCollider,
        bool& masterColliderGrabbed)
    {
        inputManager.getAction("ColliderPointPick").connect(
            [editorTriggers, &scene, &cursor, &selectedMasterCollider, &masterColliderGrabbed, &colliderPtGrabbed]
        (const Input::InputActionEvent& event)
        {
            Transform::UnitVector pixelCamera = scene.getCamera()->getPosition().to<Transform::Units::WorldPixels>();
            Transform::UnitVector cursCoord(cursor.getX() + pixelCamera.x, cursor.getY() + pixelCamera.y, Transform::Units::WorldPixels);
            if (colliderPtGrabbed == -1 && scene.getColliderPointByPosition(cursCoord).first != nullptr)
            {
                std::pair<Collision::PolygonalCollider*, int> selectedPtCollider;
                selectedPtCollider = scene.getColliderPointByPosition(cursCoord);
                if (selectedMasterCollider != nullptr && selectedMasterCollider != selectedPtCollider.first)
                {
                    selectedMasterCollider->setSelected(false);
                    selectedMasterCollider = nullptr;
                    masterColliderGrabbed = false;
                    colliderPtGrabbed = -1;
                }
                Debug::Log->debug("Action <ColliderPointPicked> triggered");
                selectedMasterCollider = selectedPtCollider.first;
                selectedMasterCollider->setSelected(true);
                colliderPtGrabbed = selectedPtCollider.second;
                editorTriggers->pushParameter("ColliderPointPicked", "collider", selectedMasterCollider);
                editorTriggers->pushParameter("ColliderPointPicked", "pointIndex", colliderPtGrabbed);
                editorTriggers->pushParameter("ColliderPointPicked", "pos", cursCoord);
                editorTriggers->trigger("ColliderPointPicked");
            }
        });
            
        inputManager.getAction("ColliderPointMove").connect(
            [editorTriggers, &selectedMasterCollider, &masterColliderGrabbed, &colliderPtGrabbed, &scene, &cursor]
        (const Input::InputActionEvent& event)
        {
            Debug::Log->debug("Action <ColliderPointMoved (pre)> triggered");
            if (selectedMasterCollider != nullptr && !masterColliderGrabbed && colliderPtGrabbed != -1)
            {
                Debug::Log->debug("Action <ColliderPointMoved> triggered");
                Transform::UnitVector pixelCamera = scene.getCamera()->getPosition().to<Transform::Units::WorldPixels>();
                Transform::UnitVector cursCoord(cursor.getConstrainedX() + pixelCamera.x, cursor.getConstrainedY() + pixelCamera.y, Transform::Units::WorldPixels);
                editorTriggers->pushParameter("ColliderPointMoved", "collider", selectedMasterCollider);
                editorTriggers->pushParameter("ColliderPointMoved", "pointIndex", colliderPtGrabbed);
                editorTriggers->pushParameter("ColliderPointMoved", "oldPos", selectedMasterCollider->get(colliderPtGrabbed).to<Transform::Units::WorldPixels>());
                editorTriggers->pushParameter("ColliderPointMoved", "pos", cursCoord);
                editorTriggers->trigger("ColliderPointMoved");
                selectedMasterCollider->get(colliderPtGrabbed).set(cursCoord);
            }
        });

        inputManager.getAction("ColliderPointRelease").connect([editorTriggers, &colliderPtGrabbed, &selectedMasterCollider](const Input::InputActionEvent& event)
        {
            editorTriggers->pushParameter("ColliderPointReleased", "collider", selectedMasterCollider);
            editorTriggers->pushParameter("ColliderPointReleased", "pointIndex", colliderPtGrabbed);
            editorTriggers->trigger("ColliderPointReleased");
            colliderPtGrabbed = -1;
        });

        inputManager.getAction("ColliderMasterPick").connect(
            [editorTriggers, &scene, &cursor, &selectedMasterCollider, &colliderPtGrabbed, &masterColliderGrabbed]
        (const Input::InputActionEvent& event)
        {

            Transform::UnitVector pixelCamera = scene.getCamera()->getPosition().to<Transform::Units::WorldPixels>();
            Transform::UnitVector cursCoord(cursor.getX() + pixelCamera.x, cursor.getY() + pixelCamera.y, Transform::Units::WorldPixels);
            if (scene.getColliderByCentroidPosition(cursCoord) != nullptr)
            {
                Collision::PolygonalCollider* tempCol = scene.getColliderByCentroidPosition(cursCoord);
                if (selectedMasterCollider != nullptr && selectedMasterCollider != tempCol)
                {
                    selectedMasterCollider->setSelected(false);
                    selectedMasterCollider = nullptr;
                    masterColliderGrabbed = false;
                    colliderPtGrabbed = -1;
                }
                Debug::Log->debug("Action <ColliderMasterPick> triggered");
                selectedMasterCollider = tempCol;
                selectedMasterCollider->setSelected(true);
                if (selectedMasterCollider->getParentId() != "") scene.getGameObject(selectedMasterCollider->getParentId())->setUpdateState(false);
                masterColliderGrabbed = true;
                editorTriggers->pushParameter("ColliderPicked", "collider", selectedMasterCollider);
                editorTriggers->pushParameter("ColliderPicked", "pos", cursCoord);
                editorTriggers->trigger("ColliderPicked");
            }
        });

        //Collision Master Move
        inputManager.getAction("ColliderMasterMove").connect(
            [editorTriggers, &selectedMasterCollider, &masterColliderGrabbed, &scene, &cursor]
        (const Input::InputActionEvent& event)
        {
            if (selectedMasterCollider != nullptr && masterColliderGrabbed)
            {
                Debug::Log->debug("Action <ColliderMoved> triggered");
                Transform::UnitVector pixelCamera = scene.getCamera()->getPosition().to<Transform::Units::WorldPixels>();
                Transform::UnitVector cursCoord(cursor.getConstrainedX() + pixelCamera.x, cursor.getConstrainedY() + pixelCamera.y, Transform::Units::WorldPixels);
                editorTriggers->pushParameter("ColliderMoved", "collider", selectedMasterCollider);
                editorTriggers->pushParameter("ColliderMoved", "oldPos", selectedMasterCollider->getCentroid().to<Transform::Units::WorldPixels>());
                editorTriggers->pushParameter("ColliderMoved", "pos", cursCoord);
                editorTriggers->trigger("ColliderMoved");
                selectedMasterCollider->setPositionFromCentroid(cursCoord);
            }
        });

        inputManager.getAction("ColliderMasterRelease").connect(
            [editorTriggers, &masterColliderGrabbed, &selectedMasterCollider, &scene]
        (const Input::InputActionEvent& event)
        {
            if (masterColliderGrabbed)
            {
                Debug::Log->debug("Action <ColliderMasterRelease> triggered");
                masterColliderGrabbed = false;
                if (selectedMasterCollider->getParentId() != "") 
                    scene.getGameObject(selectedMasterCollider->getParentId())->setUpdateState(true);
                editorTriggers->pushParameter("ColliderReleased", "collider", selectedMasterCollider);
                editorTriggers->trigger("ColliderReleased");
            }
        });

        inputManager.getAction("ColliderPointCreate").connect(
            [editorTriggers, &selectedMasterCollider, &masterColliderGrabbed, &scene, &cursor]
        (const Input::InputActionEvent& event)
        {
            if (selectedMasterCollider != nullptr && !masterColliderGrabbed)
            {
                const Transform::UnitVector pTolerance = Transform::UnitVector(6, 6, Transform::Units::WorldPixels);
                Transform::UnitVector pixelCamera = scene.getCamera()->getPosition().to<Transform::Units::WorldPixels>();
                Transform::UnitVector cursCoord(cursor.getConstrainedX() + pixelCamera.x, cursor.getConstrainedY() + pixelCamera.y, Transform::Units::WorldPixels);
                
                if (!selectedMasterCollider->getPointAroundPosition(cursCoord, pTolerance).has_value())
                {
                    Debug::Log->debug("Action <ColliderPointCreate> triggered");
                    auto newSegment = selectedMasterCollider->findClosestLine(cursCoord);
                    editorTriggers->pushParameter("ColliderPointCreated", "collider", selectedMasterCollider);
                    editorTriggers->pushParameter("ColliderPointCreated", "pointIndex", newSegment.first.index);
                    editorTriggers->pushParameter("ColliderPointCreated", "pos", cursCoord);
                    editorTriggers->trigger("ColliderPointCreated");
                    selectedMasterCollider->addPoint(cursCoord, newSegment.first.index);
                }
            }
        });

        inputManager.getAction("ColliderPointRemove").connect(
            [editorTriggers, &selectedMasterCollider, &masterColliderGrabbed, &colliderPtGrabbed, &scene]
        (const Input::InputActionEvent& event)
        {
            if (selectedMasterCollider != nullptr && !masterColliderGrabbed && colliderPtGrabbed != -1)
            {
                selectedMasterCollider->get(colliderPtGrabbed).remove();
                if (selectedMasterCollider->getPointsAmount() <= 2)
                {
                    Debug::Log->debug("Action <ColliderRemoved (from pt)> triggered");
                    editorTriggers->pushParameter("ColliderRemoved", "id", selectedMasterCollider->getId());
                    editorTriggers->trigger("ColliderRemoved");
                    selectedMasterCollider->setSelected(false);
                    scene.removeCollider(selectedMasterCollider->getId());
                    selectedMasterCollider = nullptr;
                    masterColliderGrabbed = false;
                    colliderPtGrabbed = -1;
                }
                else
                {
                    Debug::Log->debug("Action <ColliderPointRemoved> triggered");
                    editorTriggers->pushParameter("ColliderPointRemoved", "collider", selectedMasterCollider);
                    editorTriggers->pushParameter("ColliderPointRemoved", "pointIndex", colliderPtGrabbed);
                    editorTriggers->trigger("ColliderPointRemoved");
                }
                colliderPtGrabbed = -1;
                    
            }
        });

        inputManager.getAction("ColliderRelease").connect(
            [editorTriggers, &selectedMasterCollider, &scene, &cursor, &masterColliderGrabbed, &colliderPtGrabbed]
        (const Input::InputActionEvent& event)
        {
            if (selectedMasterCollider != nullptr)
            {
                Transform::UnitVector pixelCamera = scene.getCamera()->getPosition().to<Transform::Units::WorldPixels>();
                Transform::UnitVector cursCoord(cursor.getConstrainedX() + pixelCamera.x, cursor.getConstrainedY() + pixelCamera.y, Transform::Units::WorldPixels);
                if (scene.getColliderByCentroidPosition(cursCoord) == nullptr)
                {
                    if (scene.getColliderPointByPosition(cursCoord).first == nullptr)
                    {
                        Debug::Log->debug("Action <ColliderRelease> triggered");
                        editorTriggers->pushParameter("ColliderReleased", "collider", selectedMasterCollider);
                        editorTriggers->pushParameter("ColliderReleased", "pos", cursCoord);
                        editorTriggers->trigger("ColliderReleased");
                        selectedMasterCollider->setSelected(false);
                        selectedMasterCollider = nullptr;
                        masterColliderGrabbed = false;
                        colliderPtGrabbed = -1;
                    }
                }
            }
        });

        inputManager.getAction("ColliderRemove").connect(
            [editorTriggers, &selectedMasterCollider, &masterColliderGrabbed, &scene, &colliderPtGrabbed]
        (const Input::InputActionEvent& event)
        {
            if (selectedMasterCollider != nullptr && masterColliderGrabbed)
            {
                Debug::Log->debug("Action <ColliderRemove> triggered");
                editorTriggers->pushParameter("ColliderRemoved", "id", selectedMasterCollider->getId());
                editorTriggers->trigger("ColliderRemoved");
                selectedMasterCollider->setSelected(false);
                scene.removeCollider(selectedMasterCollider->getId());
                selectedMasterCollider = nullptr;
                masterColliderGrabbed = false;
                colliderPtGrabbed = -1;
            }
        });

        inputManager.getAction("ColliderCreate").connect([editorTriggers, &selectedMasterCollider, &scene, &cursor](const Input::InputActionEvent& event)
        {
            if (selectedMasterCollider == nullptr)
            {
                Debug::Log->debug("Action <ColliderCreate> triggered");
                Transform::UnitVector pixelCamera = scene.getCamera()->getPosition().to<Transform::Units::WorldPixels>();
                Transform::UnitVector pPos(cursor.getConstrainedX(), cursor.getConstrainedY(), Transform::Units::WorldPixels);
                pPos += pixelCamera;
                Collision::PolygonalCollider* newCollider = scene.createCollider();
                newCollider->addPoint(Transform::UnitVector(50, 0, Transform::Units::WorldPixels));
                newCollider->addPoint(Transform::UnitVector(0, 50, Transform::Units::WorldPixels));
                newCollider->addPoint(Transform::UnitVector(100, 50, Transform::Units::WorldPixels));
                newCollider->setPositionFromCentroid(pPos);
                editorTriggers->pushParameter("ColliderCreated", "collider", newCollider);
                editorTriggers->pushParameter("ColliderCreated", "pos", pPos);
                editorTriggers->trigger("ColliderCreated");
            }
        });
    }
}

#include "qgeojson.h"

/*! \class QGeoJson
    \inmodule Qt.labs.location
    \ingroup json
    \ingroup shared
    \reentrant
    \since WIP

    \brief The QGeoJson class provides a way to import and export a GeoJSON documents.

    QGeoJson is a class that read GeoJSON document using the toJsonDocument methods and can import and
    export this document in/from a QVariantMap build up like follows:

    The class has 2 public methods:

    QVariantMap importGeoJson(const QJsonDocument &geojsonDoc);
    QJsonDocument exportGeoJson(const QVariantMap &geojsonMap);
    the QvariantMap returned by the importer has a very specific architecture, shaped on the GeoJSON structure. The exporter accepts a QVariantMap with the same structure
    This QVariantMap always has at least one (key, value) pair.
    The key is QString containing one of the 9 GeoJson object types, which are:

    7 Geometry Object Types:
    - “Point”
    - “MultiPoint”
    - “LineString”
    - “MultiLineString”
    - “Polygon”
    - “MultiPolygon”
    - “GeometryCollection”

    2 Non-Geometry GeoJSON Object Types:
    - “Feature”
    - “FeatureCollection”

    The value crresponding to this kye has a different format depending on the type:

    Single Type Geometry Objects:
    Key: “Point”
    The value contains a QGeoCircle, The coordinates of the original GeoJSON point are stored in the center of this QGeoCircle

    Key: “LineString”
    The value contains a QGeoPath, The coordinates of the GeoJSON points included in the original LineString are stored in the path of the QGeoPath.

    Key: “Polygon”
    The value contains a QGeoPolygon, The coordinates of the GeoJSON points representing the outer perimeter are stored in the path of the QGeoPolygon, if holes are present, the coordinates are stored in the QGeoPolygon using the proper methods.

    Homogeneously Typed Multipart Geometry Objects:
    Key: “MultiPoint”
    The value contains a QList<QGeoCircle>, The coordinates of the original GeoJSON points are stored in the center of these QGeoCircles

    Key: “MultiLineString”
    The value contains a QList<QGeoPath>, each structured like explained in the LineString section.

    Key: “MultiPolygon”
    The value contains a QList<QGeoPolygon>, each QgeoPolygon is structured like explained in the Polygon section.

    Heterogeneous composition of the other geometry types
    Key: “GeometryCollection”
    The value contains a QVariantList, each element of this list is a QVariantMap corresponding to one of the above listed GeoJson Geometry types, plus the GeometryCollection type itself, albeit GeoJson RFC advises against nesting GeometryCollections.

    Objects including geometries and attributes for an entity
    Key: “Feature”
    The value contains a QVariantMap, This map has a (key, value) pair corresponding the the pair of QVariantMap originated from one of the 7 GeoJSON geometry types already described. One more (key, value) pair conains a “properties” key and an additional QVariantMap as value. The (key, value) pairs of this last properties Map are pairs of Qstrings

    Key: “FeatureCollection”
    The value contains a QList<QVariantMap>, each element of this list is a “Feature” type QVariantMap

    A GeoJSON document can be converted from its text-based representation to a QJVariantMap
    using GeoJson::importGeoJson().

    Validity of the parsed document can be queried with !isNull() or using external API's.

static QGeoCircle importPoint(const QVariantMap &point);
static QGeoPath importLineString(const QVariantMap &lineString);
static QGeoPolygon importPolygon(const QVariantMap &polygon);

static QVariantList importMultiPoint(const QVariantMap &multiPoint); //TODO: change the returned value to QVariantList
static QVariantList importMultiLineString(const QVariantMap &multiLineString);
static QVariantList importMultiPolygon(const QVariantMap &multiPolygon);

static QVariantList importGeometryCollection(const QVariantMap &geometryCollection);
static QVariantMap importFeature(const QVariantMap &feature);
static QVariantList importFeatureCollection(const QVariantMap &featureCollection);

static QGeoCoordinate importPointCoordinates(const QVariant &obtainedCoordinates); // converts a single position
static QList<QGeoCoordinate> importLineStringCoordinates(const QVariant &obtainedCoordinates); // converts an array of positions
static QList<QList<QGeoCoordinate>> importPolygonCoordinates(const QVariant &obtainedCoordinates); // converts an array of array of positions
static QVariantMap importGeometry(const QVariantMap &geometryMap);

static QJsonObject exportPoint(const QVariantMap &point); // generates a QJsonObject from a QVariantMap with a key/value: "Point"/QGeoCircle
static QJsonObject exportLineString(const QVariantMap &lineString);
static QJsonObject exportPolygon(const QVariantMap &polygon);

static QJsonObject exportMultiPoint(const QVariantMap &multiPoint);
static QJsonObject exportMultiLineString(const QVariantMap &multiLineString);
static QJsonObject exportMultiPolygon(const QVariantMap &multiPolygon);

static QJsonValue exportPointCoordinates(const QGeoCoordinate &obtainedCoordinates);
static QJsonValue exportLineStringCoordinates(const QList <QGeoCoordinate> &obtainedCoordinatesList);
static QJsonValue exportPolygonCoordinates(const QList<QList<QGeoCoordinate>> &obtainedCoordinates);
static QJsonObject exportGeometry(const QVariantMap &geometryMap);

static QJsonObject exportFeature(const QVariantMap &feature);
static QJsonObject exportGeometryCollection(const QVariantMap &geometryCollection);
static QJsonObject exportFeatureCollection(const QVariantMap &featureCollection);
*/

/*!
    \internal
 */
QGeoCircle QGeoJson::importPoint(const QVariantMap &pointMap)
{
    const QString keyCoord = "coordinates";
    QGeoCircle parsedPoint;

    QGeoCoordinate center;
    QVariant valueCoords = pointMap.value(keyCoord); // returns the value associated with the key coordinates (Point)
    center = importPointCoordinates(valueCoords);
    parsedPoint.setCenter(center);
    return parsedPoint;
}

/*!
    \internal
 */
QGeoPath  QGeoJson::importLineString(const QVariantMap &lineMap)
{
    const QString keyCoord = "coordinates";
    QGeoPath parsedLineString;

    QList <QGeoCoordinate> coordinatesList;
    QVariant valueCoordinates = lineMap.value(keyCoord); // returns the value associated with the key coordinates (LineString)
    coordinatesList = importLineStringCoordinates(valueCoordinates); // import an array of QGeoCoordinate from a nested GeoJSON array
    parsedLineString.setPath(coordinatesList);
    return parsedLineString;
}

/*!
    \internal
 */
QGeoPolygon QGeoJson::importPolygon(const QVariantMap &polyMap)
{
    int i = 0; // meant to bypass the lack of iterator position tracking
    const QString keyCoord = "coordinates";
    QGeoPolygon parsedPolygon;

    QList<QList<QGeoCoordinate>> perimeters;
    QList<QList<QGeoCoordinate>> ::iterator iter;
    QVariant valueCoordinates = polyMap.value(keyCoord); // returns the value associated with the key coordinates (Polygon)
    perimeters = importPolygonCoordinates(valueCoordinates); // import an array of QList<QGeocoordinates>
    for (iter = perimeters.begin(); iter != perimeters.end(); ++iter) {
        if (i == 0)
            parsedPolygon.setPath(*iter); // external perimeter
        else
            parsedPolygon.addHole(*iter); // inner perimeters
        i++;
    }
    return parsedPolygon;
}

/*!
    \internal
 */
QVariantList QGeoJson::importMultiPoint(const QVariantMap &multiPointMap)
{
    const QString keyCoord = "coordinates";
    QVariantList parsedMultiPoint;

    QGeoCircle parsedPoint;
    QGeoCoordinate coordinatesCenter;

    QVariant listCoords = multiPointMap.value(keyCoord);
    QVariantList list = listCoords.value<QVariantList>();
    QVariantList::iterator iter; // iterating MultiPoint coordinates nasted arrays
    for (iter = list.begin(); iter != list.end(); ++iter) {
        coordinatesCenter = importPointCoordinates(*iter);
        parsedPoint.setCenter(coordinatesCenter);
        parsedMultiPoint.append(QVariant::fromValue(parsedPoint)); // adding the newly created QGeoCircle to the dastination QVariantList
    }
    return parsedMultiPoint;
}

/*!
    \internal
 */
QVariantList QGeoJson::importMultiLineString(const QVariantMap &multiLineStringMap)
{
    QGeoPath parsedLineString;
    QVariantList parsedMultiLineString;

    QList <QGeoCoordinate> coordinatesList;
    QString keyCoord = "coordinates";
    QVariant listCoords = multiLineStringMap.value(keyCoord);
    QVariantList list = listCoords.value<QVariantList>();

    QVariantList::iterator iter; // iterating the MultiLineString coordinates nasted arrays using importLineStringCoordinates
    for (iter = list.begin(); iter != list.end(); ++iter) {
        coordinatesList = importLineStringCoordinates(*iter);
        parsedLineString.setPath(coordinatesList);
        parsedMultiLineString.append(QVariant::fromValue(parsedLineString));
    }
    return parsedMultiLineString;
}

/*!
    \internal
 */
QVariantList QGeoJson::importMultiPolygon(const QVariantMap &multiPolyMap)
{
    const QString keyCoord = "coordinates";
    QVariantList parsedMultiPoly;

    QList<QList<QGeoCoordinate>> coordinatesList;
    QList<QList<QGeoCoordinate>>::iterator iter;
    QGeoPolygon singlePoly;
    QVariant valueCoordinates = multiPolyMap.value(keyCoord);

    QVariantList list = valueCoordinates.value<QVariantList>();
    for (const QVariant &polyVariantCoords: list) {
        coordinatesList = importPolygonCoordinates(polyVariantCoords);
        for (iter = coordinatesList.begin(); iter != coordinatesList.end(); ++iter) {
            if (coordinatesList.indexOf(*iter) == 0)
                singlePoly.setPath(*iter);
            else
                singlePoly.addHole(*iter);
        }
        parsedMultiPoly << QVariant::fromValue(singlePoly);
    }
    return parsedMultiPoly;
}

/*!
    \internal
 */
QGeoCoordinate QGeoJson::importPointCoordinates(const QVariant &obtainedCoordinates)
{
    QGeoCoordinate parsedCoordinates;
    QVariantList list = obtainedCoordinates.value<QVariantList>();

    QVariantList::iterator iter; // iterating Point coordinates arrays
    int i = 0;
    for (iter = list.begin(); iter != list.end(); ++iter) {
        QString elementType = iter->typeName();
        switch (i) {
        case 0:
            parsedCoordinates.setLatitude(iter->toDouble());
            break;
        case 1:
            parsedCoordinates.setLongitude(iter->toDouble());
            break;
        case 2:
            parsedCoordinates.setAltitude(iter->toDouble());
            break;}
        i++;
    }
    return parsedCoordinates;
}

/*!
    \internal
 */
QList <QGeoCoordinate> QGeoJson::importLineStringCoordinates(const QVariant &obtainedCoordinates)
{
    QList <QGeoCoordinate> parsedCoordinatesLine;
    QVariantList list2 = obtainedCoordinates.value<QVariantList>();
    QGeoCoordinate pointForLine;

    QVariantList::iterator iter; // iterating the LineString coordinates nasted arrays
    for (iter = list2.begin(); iter != list2.end(); ++iter) {
        pointForLine = importPointCoordinates(*iter); // using importPointCoordinates method to convert multi positions from QVariant containing QVariantList
        parsedCoordinatesLine.append(pointForLine); // populating the QList of coordinates
    }
    return parsedCoordinatesLine;
}

/*!
    \internal
 */
QList<QList<QGeoCoordinate>> QGeoJson::importPolygonCoordinates(const QVariant &obtainedCoordinates)
{
    QList<QList<QGeoCoordinate>> parsedCoordinatesPoly;
    QVariantList obtainedCoordinatesList = obtainedCoordinates.value<QVariantList>(); // extracting the QVariantList

    QList<QGeoCoordinate> coordinatesList; // iterating the Polygon coordinates nasted arrays
    for (const QVariant &coordinatesVariant: obtainedCoordinatesList) {
        coordinatesList = importLineStringCoordinates(coordinatesVariant);
        parsedCoordinatesPoly << coordinatesList;
    }
    return parsedCoordinatesPoly;
}

/*!
    \internal
 */
QVariantList QGeoJson::importGeometryCollection(const QVariantMap &geometryCollection)
{
    QVariantList parsedGeoCollection;
    QVariant listGeometries = geometryCollection.value("geometries");
    QVariantList list = listGeometries.value<QVariantList>();

    QVariantList::iterator iterGeometries;
    for (iterGeometries = list.begin(); iterGeometries != list.end(); ++ iterGeometries) {
        QVariantMap geometryMap = iterGeometries->value<QVariantMap>();
        QVariantMap geoMap = importGeometry(geometryMap);
        parsedGeoCollection.append(geoMap);
    }
    return parsedGeoCollection;
}

/*!
    \internal
 */
QVariantMap QGeoJson::importFeature(const QVariantMap &feature)
{
    QVariantMap parsedFeature;
    QVariant featureGeometry = feature.value("geometry"); // extracting GeoJson "geometry" member from the QVariantMap
    QVariantMap mapGeometry = featureGeometry.value<QVariantMap>();
    QVariantMap geoMap = importGeometry(mapGeometry);

    QVariant elementValue = QVariant::fromValue(geoMap);
    QString elementKey = "geometry";
    parsedFeature.insert(elementKey, elementValue);

    elementValue = feature.value("properties");  // extracting GeoJson "properties" member from the QVariantMap
    elementKey = "properties";
    parsedFeature.insert(elementKey, elementValue);

    elementValue = feature.value("id"); // extracting GeoJson "id" member from the QVariantMap
    if (elementValue != QVariant::Invalid) {
        elementKey = "id";
        parsedFeature.insert(elementKey, elementValue);
    }
    return parsedFeature;
}

/*!
    \internal
 */
QVariantList QGeoJson::importFeatureCollection(const QVariantMap &featureCollection)
{
    QVariantList parsedFeatureCollection;
    QVariant featureVariant = featureCollection.value("features");
    QVariantList featureVariantList = featureVariant.value<QVariantList>();

    QVariantMap importedMap;
    for (const QVariant &singleVariantFeature: featureVariantList) {
        QVariantMap featureMap = singleVariantFeature.value<QVariantMap>();
        QVariantMap featMap = importFeature(featureMap);
        importedMap.insert("Feature",featMap);
        parsedFeatureCollection.append(importedMap);
    }
    return parsedFeatureCollection;
}

/*!
    \internal
 */
QVariantMap QGeoJson::importGeometry(const QVariantMap &geometryMap)
{
    QVariantMap parsedGeoJsonMap;
    QString geometryTypes[] = {
        "Point",
        "MultiPoint",
        "LineString",
        "MultiLineString",
        "Polygon",
        "MultiPolygon","GeometryCollection"
    };

    enum geoTypeSwitch {
        Point,
        MultiPoint,
        LineString,
        MultiLineString,
        Polygon,
        MultiPolygon,
        GeometryCollection
    };

    const QString geoKey = geoKey.toString();
    const QString keyGeometries = "geometries";
    QVariant geoValue = geometryMap.value("type");

    int len = sizeof(geometryTypes)/sizeof(*geometryTypes);
    int i = 0;

    for (i = 0; i<len-1; i++) {
        if (geoValue == geometryTypes[i])
            break;
    }

    switch (i) {
    case Point:
    {
        const QString geoKey = "Point";
        QGeoCircle circle = importPoint(geometryMap);
        QVariant geoValue = QVariant::fromValue(circle);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case MultiPoint:
    {
        const QString geoKey = "MultiPoint"; // creating the key for the first element of the QVariantMap that will be returned
        QVariantList multiCircle = importMultiPoint(geometryMap);
        QVariant geoValue = QVariant::fromValue(multiCircle); // wraps up the multiCircle item in a QVariant
        parsedGeoJsonMap.insert(geoKey, geoValue); // creating the QVariantMap element
        break;
    }
    case LineString:
    {
        const QString geoKey = "LineString";
        QGeoPath lineString = importLineString(geometryMap);
        QVariant geoValue = QVariant::fromValue(lineString);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case MultiLineString:
    {
        const QString geoKey = "MultiLineString";
        QVariantList multiLineString = importMultiLineString(geometryMap);
        QVariant geoValue = QVariant::fromValue(multiLineString);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case Polygon:
    {
        const QString geoKey = "Polygon";
        QGeoPolygon poly = importPolygon(geometryMap);
        QVariant geoValue = QVariant::fromValue(poly);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case MultiPolygon:
    {
        const QString geoKey = "MultiPolygon";
        QVariantList multiPoly = importMultiPolygon(geometryMap);
        QVariant geoValue = QVariant::fromValue(multiPoly);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case GeometryCollection: // list of GeoJson geometry objects
    {
        const QString geoKey = "GeometryCollection";
        QVariantList multigeo = importGeometryCollection(geometryMap);
        QVariant geoValue = QVariant::fromValue(multigeo);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    }
    return parsedGeoJsonMap;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportPoint(const QVariantMap &pointMap)
{
    const QString keyType = "type";
    const QString keyCoord = "coordinates";

    QJsonObject parsedPoint;
    QJsonValue type = "Point";

    QVariant geoCircle = pointMap.value(type.toString()); // exporting the value of "Point" QVariantMap node in a QVariant
    QGeoCircle circle = geoCircle.value<QGeoCircle>();
    QGeoCoordinate center = circle.center();

    parsedPoint.insert(keyType,type);
    parsedPoint.insert(keyCoord, exportPointCoordinates(center));
    return parsedPoint;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportLineString(const QVariantMap &lineStringMap)
{
    const QString keyType = "type";
    const QString keyLineString = "coordinates";

    QJsonObject parsedMultiPoint;

    QJsonValue lineCoordinates;
    QJsonValue typeValue = "LineString";

    QList <QGeoCoordinate> lineCoordinatesList;
    QVariant pathVariant = lineStringMap.value(typeValue.toString());

    QGeoPath parsedPath;
    parsedPath = pathVariant.value<QGeoPath>();

    lineCoordinatesList = parsedPath.path();
    lineCoordinates = exportLineStringCoordinates(lineCoordinatesList);

    parsedMultiPoint.insert(keyType, typeValue);
    parsedMultiPoint.insert(keyLineString, lineCoordinates);
    return parsedMultiPoint;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportPolygon(const QVariantMap &polygonMap)
{
    const QString keyType = "type";
    const QString keyCoord = "coordinates";

    QJsonObject parsedPolygon;

    QGeoPolygon parsedPoly;

    QJsonValue typeValue = "Polygon";
    QJsonValue polyCoordinates;

    QList<QList<QGeoCoordinate>> obtainedCoordinatesPoly;
    QVariant polygonVariant = polygonMap.value(typeValue.toString());

    parsedPoly = polygonVariant.value<QGeoPolygon>();
    obtainedCoordinatesPoly << parsedPoly.path();

    if (parsedPoly.holesCount()!=0)
        for (int i = 0; i<parsedPoly.holesCount(); i++) {
            obtainedCoordinatesPoly << parsedPoly.holePath(i);
        }
    polyCoordinates = exportPolygonCoordinates(obtainedCoordinatesPoly);

    parsedPolygon.insert(keyType, typeValue);
    parsedPolygon.insert(keyCoord, polyCoordinates);
    return parsedPolygon;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportMultiPoint(const QVariantMap &multiPointMap)
{
    const QString keyType = "type";
    const QString keyCoord = "coordinates";

    QJsonObject parsedMultiPoint;

    QJsonValue typeValue = "MultiPoint";
    QJsonValue singlePosition;
    QJsonValue multiPosition;

    QList <QGeoCoordinate> obtainedCoordinatesMP;
    QVariant multiCircleVariant = multiPointMap.value(typeValue.toString());
    QVariantList multiCircleVariantList = multiCircleVariant.value<QVariantList>();

    for (const QVariant &exCircle: multiCircleVariantList) {
        obtainedCoordinatesMP << exCircle.value<QGeoCircle>().center();
    }
    multiPosition = exportLineStringCoordinates(obtainedCoordinatesMP);

    parsedMultiPoint.insert(keyType, typeValue);
    parsedMultiPoint.insert(keyCoord, multiPosition);
    return parsedMultiPoint;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportMultiLineString(const QVariantMap &multiLineStringMap)
{
    const QString keyType = "type";
    const QString keyCoord = "coordinates";

    QJsonObject parsedMultiLineString;

    QJsonValue typeValue = "MultiLineString";

    QList<QList<QGeoCoordinate>> obtainedCoordinatesMLS;
    QVariant multiPathVariant = multiLineStringMap.value(typeValue.toString());
    QVariantList multiPathList = multiPathVariant.value<QVariantList>();

    for (const QVariant &singlePath: multiPathList ){
        obtainedCoordinatesMLS << singlePath.value<QGeoPath>().path();
    }

    parsedMultiLineString.insert(keyType, typeValue);
    parsedMultiLineString.insert(keyCoord, exportPolygonCoordinates(obtainedCoordinatesMLS));
    return parsedMultiLineString;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportMultiPolygon(const QVariantMap &multiPolygonMap)
{
    const QString keyType = "type";
    const QString keyCoord = "coordinates";

    QJsonObject parsedMultiPolygon;

    QJsonValue typeValue = "MultiPolygon";
    QJsonValue polyCoordinates;

    QJsonArray parsedArrayPolygon;
    QList<QList<QGeoCoordinate>> obtainedPolyCoordinates;

    QVariant multiPolygonVariant = multiPolygonMap.value(typeValue.toString());
    QVariantList multiPolygonList = multiPolygonVariant.value<QVariantList>();

    int polyHoles=0;
    int currentHole;

    for (const QVariant &singlePoly: multiPolygonList ) { //start parsing polygon list
        obtainedPolyCoordinates << singlePoly.value<QGeoPolygon>().path(); //extract external polygon path
        polyHoles = singlePoly.value<QGeoPolygon>().holesCount();
        if (polyHoles) //check if the polygon has holes
            for (currentHole = 0 ; currentHole<polyHoles; currentHole++)
                obtainedPolyCoordinates << singlePoly.value<QGeoPolygon>().holePath(currentHole);
        polyCoordinates = exportPolygonCoordinates(obtainedPolyCoordinates); //generates QJsonDocument compatible value
        parsedArrayPolygon.append(polyCoordinates); //adds one level of nesting in coordinates
        obtainedPolyCoordinates.clear(); //clears the temporary polygon linear ring storage
    }
    QJsonValue parsed = parsedArrayPolygon;

    parsedMultiPolygon.insert(keyType, typeValue);
    parsedMultiPolygon.insert(keyCoord, parsed);
    return parsedMultiPolygon;
}

/*!
    \internal
 */
QJsonValue QGeoJson::exportPointCoordinates(const QGeoCoordinate &obtainedCoordinates)
{
    QJsonValue geoLat = obtainedCoordinates.latitude();
    QJsonValue geoLong = obtainedCoordinates.longitude();

    QJsonArray array = {geoLat, geoLong};
    QJsonValue geoAlt;

    if (!qIsNaN(obtainedCoordinates.altitude())) {
        geoAlt = obtainedCoordinates.altitude();
        array.append(geoAlt);
    }
    QJsonValue geoArray = array;
    return geoArray;
}

/*!
    \internal
 */
QJsonValue QGeoJson::exportLineStringCoordinates(const QList<QGeoCoordinate> &obtainedCoordinatesList)
{
    QJsonValue lineCoordinates;
    QJsonValue multiPosition;
    QJsonArray arrayPosition;

    for (const QGeoCoordinate &pointCoordinates: obtainedCoordinatesList) {
        multiPosition = exportPointCoordinates(pointCoordinates);
        arrayPosition.append(multiPosition);
    }
    lineCoordinates = arrayPosition;
    return lineCoordinates;
}

/*!
    \internal
 */
QJsonValue QGeoJson::exportPolygonCoordinates(const QList<QList<QGeoCoordinate>> &obtainedCoordinates)
{
    QJsonValue lineCoordinates;
    QJsonValue polyCoordinates;
    QJsonArray arrayPath;
    for (const QList<QGeoCoordinate> &parsedPath: obtainedCoordinates) {
        lineCoordinates = exportLineStringCoordinates(parsedPath); //method to extract an array of QGeoCoordinate from a nested GeoJSON array
        arrayPath.append(lineCoordinates);
    }
    polyCoordinates = arrayPath;
    return polyCoordinates;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportGeometry(const QVariantMap &geometryMap)
{
    QJsonObject newObject;
    QJsonDocument newDocument;
    if (geometryMap.contains("Point")) // check if the map contains the key Point
        newObject = exportPoint(geometryMap);
    if (geometryMap.contains("MultiPoint")) // check if the map contains the key MultiPoint
        newObject = exportMultiPoint(geometryMap);
    if (geometryMap.contains("LineString"))
        newObject = exportLineString(geometryMap);
    if (geometryMap.contains("MultiLineString"))
        newObject = exportMultiLineString(geometryMap);
        newDocument.setObject(newObject);
    if (geometryMap.contains("Polygon"))
        newObject = exportPolygon(geometryMap);
    if (geometryMap.contains("MultiPolygon"))
        newObject = exportMultiPolygon(geometryMap);
    if (geometryMap.contains("GeometryCollection"))
        newObject = exportGeometryCollection(geometryMap);
    return newObject;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportFeature(const QVariantMap &feature)
{
    const QString keyType = "type";
    const QString keyFeature = "geometry";
    const QString keyProp = "properties";
    const QString keyId = "id";

    QJsonObject parsedFeature;
    QJsonObject exportedFeature;
    QJsonObject exportedProp;
    QJsonObject valueProp;

    QVariantMap featureMap;
    QVariantMap feat;

    QJsonValue valueType = "Feature";
    QJsonValue valueFeature;
    QJsonValue valueProperties;
    QJsonValue valueId;

    featureMap = feature.value(valueType.toString()).value<QVariantMap>();
    feat = featureMap.value(keyFeature).value<QVariantMap>();
    exportedFeature = exportGeometry(feat);
    valueFeature = exportedFeature;

    feat = featureMap.value(keyProp).value<QVariantMap>();

    parsedFeature.insert(keyType, valueType);
    parsedFeature.insert(keyFeature, valueFeature);

    valueProp = featureMap.value(keyProp).value<QVariant>().toJsonObject();
    valueProperties = valueProp;
    parsedFeature.insert(keyProp, valueProperties);

    valueId = featureMap.value(keyId).value<QVariant>().toJsonValue();
    parsedFeature.insert(keyId, valueId);
    return parsedFeature;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportGeometryCollection(const QVariantMap &geometryCollection)
{
    const QString keyType = "type";
    const QString keyGeometries = "geometries";

    QJsonObject parsed;
    QJsonObject parsedGeometry;
    QVariantList geometriesList;

    QJsonValue valueType = "GeometryCollection";
    QJsonValue valueGeometries;
    QJsonArray parsedGeometries;

    geometriesList = geometryCollection.value(valueType.toString()).value<QVariantList>();
    for (const QVariant &extractedGeometry: geometriesList){
        parsedGeometry = exportGeometry(extractedGeometry.value<QVariantMap>());
        valueGeometries = parsedGeometry;
        parsedGeometries.append(valueGeometries);
    }
    valueGeometries = parsedGeometries;

    parsed.insert(keyType, valueType);
    parsed.insert(keyGeometries, valueGeometries);
    return parsed;
}

/*!
    \internal
 */
QJsonObject QGeoJson::exportFeatureCollection(const QVariantMap &featureCollection)
{
    const QString keyType = "type";
    const QString keyFeature = "features";

    QJsonObject parsedFeatureCollection;
    QJsonObject exportedFeature;

    QVariantMap featureMap;
    QVariantMap featfeat;

    QJsonValue valueType = "FeatureCollection";
    QJsonValue valueFeature;
    QJsonArray array;

    QVariant extractedFeatureVariant = featureCollection.value("FeatureCollection");
    QVariantList extractedFeaturVariantList = extractedFeatureVariant.value<QVariantList>();
    for (const QVariant &singleVariantFeature: extractedFeaturVariantList) {
            featureMap = singleVariantFeature.value<QVariantMap>();
            exportedFeature = exportFeature(featureMap);
            valueFeature = exportedFeature;
            array.append(valueFeature);
    }
    valueFeature = array;

    parsedFeatureCollection.insert(keyType, valueType);
    parsedFeatureCollection.insert(keyFeature, valueFeature);
    return parsedFeatureCollection;
}

/*!

*/
QVariantMap QGeoJson::importGeoJson(const QJsonDocument &importDoc)
{
    QJsonObject object = importDoc.object(); // Read json object from imported doc
    QVariantMap standardMap = object.toVariantMap(); // extraced map using Qt's API

    QString geoType[] = {
        "Point",
        "MultiPoint",
        "LineString",
        "MultiLineString",
        "Polygon",
        "MultiPolygon",
        "GeometryCollection",
        "Feature",
        "FeatureCollection"
    };

    enum geoTypeSwitch {
        Point,
        MultiPoint,
        LineString,
        MultiLineString,
        Polygon,
        MultiPolygon,
        GeometryCollection,
        Feature,
        FeatureCollection
    };

    // Checking wether the JSON object has a "type" member
    QVariant typekey = standardMap.value("type");
    if (typekey == QVariant::Invalid) {
        // qDebug() << " [x] Type check failed - Invalid GeoJSON document: no \"type\" key in the object.\n";
    }

    QString typevalue = typekey.toString();
    int len = sizeof(geoType)/sizeof(*geoType);

    int i = 0; // index of the iteration where the value of type has been veryfied

    // Checking whether the "type" member has a GeoJSON admitted value

    for (i=0; i<len-1; i++) {
        if (typevalue == geoType[i]) // [v] Type check passed
            break;
        else if (i==len-1) {} // [x] Type check failed
    }

    QVariantMap parsedGeoJsonMap;
    switch (i) {
    case Point:
    {
        const QString keyMap = "Point";
        QGeoCircle circle = importPoint(standardMap);
        QVariant valueMap = QVariant::fromValue(circle);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case MultiPoint:
    {
        const QString keyMap = "MultiPoint"; // creating the key for the first element of the QVariantMap that will be returned
        QVariantList multiCircle = importMultiPoint(standardMap);
        QVariant valueMap = QVariant::fromValue(multiCircle); // wraps up the multiCircle item in a QVariant
        QList <QGeoCircle> testlist;

        parsedGeoJsonMap.insert(keyMap, valueMap); // creating the QVariantMap element
        break;
    }
    case LineString:
    {
        const QString keyMap = "LineString";
        QGeoPath lineString = importLineString(standardMap);
        QVariant valueMap = QVariant::fromValue(lineString);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case MultiLineString:
    {
        const QString keyMap = "MultiLineString";
        QVariantList multiLineString = importMultiLineString(standardMap);
        QVariant valueMap = QVariant::fromValue(multiLineString);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case Polygon:
    {
        const QString keyMap = "Polygon";
        QGeoPolygon poly = importPolygon(standardMap);
        QVariant valueMap = QVariant::fromValue(poly);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case MultiPolygon:
    {
        const QString keyMap = "MultiPolygon";
        QVariantList multiPoly = importMultiPolygon(standardMap);
        QVariant valueMap = QVariant::fromValue(multiPoly);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case GeometryCollection: // list of GeoJson geometry objects
    {
        const QString keyMap = "GeometryCollection";
        QVariantList multiGeo = importGeometryCollection(standardMap);
        QVariant valueMap = QVariant::fromValue(multiGeo);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case Feature: // single GeoJson geometry object with properties
    {
        const QString keyMap = "Feature";
        QVariantMap feat = importFeature(standardMap);
        QVariant valueMap = QVariant::fromValue(feat);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case FeatureCollection: // heterogeneous list of GeoJSON geometries with properties
    {
        const QString keyMap = "FeatureCollection";
        QVariantList featCollection = importFeatureCollection(standardMap);
        QVariant valueMap = QVariant::fromValue(featCollection);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    default: // standardMap.value("type") It's not a valid value
    }

    // searching for the bbox member, if found, copy it to the output QVariantMap
    const QString keyMap = "bbox";
    QVariant valueMap = standardMap.value("bbox");
    if (bboxValue != QVariant::Invalid) {
        parsedGeoJsonMap.insert(keyMap, bboxValue);
    }
    return parsedGeoJsonMap;
}

/*!

*/
QJsonDocument QGeoJson::exportGeoJson(const QVariantMap &exportMap)
{
    QJsonObject newObject;
    QJsonDocument newDocument;
    if (exportMap.contains("Point")) // check if the map contains the key Point
        newObject = exportPoint(exportMap);
    if (exportMap.contains("MultiPoint"))
        newObject = exportMultiPoint(exportMap);
    if (exportMap.contains("LineString"))
        newObject = exportLineString(exportMap);
    if (exportMap.contains("MultiLineString"))
        newObject = exportMultiLineString(exportMap);
    if (exportMap.contains("Polygon"))
        newObject = exportPolygon(exportMap);
    if (exportMap.contains("MultiPolygon"))
        newObject = exportMultiPolygon(exportMap);
    if (exportMap.contains("GeometryCollection"))
        newObject = exportGeometry(exportMap);
    if (exportMap.contains("Feature"))
        newObject = exportFeature(exportMap);
    if (exportMap.contains("FeatureCollection"))
        newObject = exportFeatureCollection(exportMap);
    newDocument.setObject(newObject);
    return newDocument;
}
